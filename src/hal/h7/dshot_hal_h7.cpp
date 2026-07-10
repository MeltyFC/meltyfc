// MeltyFC — DShot HAL: STM32H7 Family (H743, H725)
// A1: Table-driven with DMAMUX routing.
// [2026-07-09] A1 refactor.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "hal/common/gpio_port_clock.h"
#include "hal/common/gpio_init.h"
#include "hal/common/timer_clock.h"
#include "dshot_common.hpp"

#ifdef STM32H7xx
#include "stm32h7xx_hal.h"
#include "target.h"

extern void meltyH7MpuInit(void);

namespace melty {
namespace hal {

static constexpr uint8_t NUM_MOTORS = 4;
static dshot::DshotTimingConfig dshotTiming;

static DMA_BUFFER_ATTR uint16_t dshotCompareBuf[NUM_MOTORS][dshot::DSHOT_COMPARE_BUF_SIZE];
static DMA_BUFFER_ATTR uint32_t telemCaptureBuf[NUM_MOTORS][32];
static volatile bool telemReady[NUM_MOTORS] = {};
static volatile uint8_t dmaActiveMask = 0;
static uint32_t dmaCommitTimestamp = 0;
static constexpr uint32_t DMA_TIMEOUT_US = 250; // R16-1
static uint32_t commitSkipCount = 0; // R16-2

static TIM_HandleTypeDef hMotorTimer[NUM_MOTORS];
static DMA_HandleTypeDef hDmaMotor[NUM_MOTORS];

struct H7MotorRoute {
    TIM_TypeDef* timer;
    uint32_t channel;
    GPIO_TypeDef* gpioPort;
    uint16_t gpioPin;
    uint8_t gpioAF;
    uint32_t dmamuxRequest;  // H7 DMAMUX — programmable routing
};

static const H7MotorRoute motorRoutes[NUM_MOTORS] = {
    { MOTOR1_TIMER, MOTOR1_CHANNEL, MOTOR1_GPIO_PORT, MOTOR1_GPIO_PIN, MOTOR1_AF, MOTOR1_DMAMUX_REQUEST },
    { MOTOR2_TIMER, MOTOR2_CHANNEL, MOTOR2_GPIO_PORT, MOTOR2_GPIO_PIN, MOTOR2_AF, MOTOR2_DMAMUX_REQUEST },
    { MOTOR3_TIMER, MOTOR3_CHANNEL, MOTOR3_GPIO_PORT, MOTOR3_GPIO_PIN, MOTOR3_AF, MOTOR3_DMAMUX_REQUEST },
    { MOTOR4_TIMER, MOTOR4_CHANNEL, MOTOR4_GPIO_PORT, MOTOR4_GPIO_PIN, MOTOR4_AF, MOTOR4_DMAMUX_REQUEST },
};

static TIM_TypeDef* initializedTimers[NUM_MOTORS] = {};
static uint8_t numInitializedTimers = 0;

static bool timerAlreadyInitialized(TIM_TypeDef* tim) {
    for (uint8_t i = 0; i < numInitializedTimers; i++)
        if (initializedTimers[i] == tim) return true;
    return false;
}

static void enableTimerClock(TIM_TypeDef* tim) {
    if (tim == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (tim == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (tim == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (tim == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
}

void dshotInit() {
    meltyH7MpuInit();

    for (int i = 0; i < NUM_MOTORS; i++) {
        DMA_BUFFER_ASSERT(dshotCompareBuf[i]);
        DMA_BUFFER_ASSERT(telemCaptureBuf[i]);
    }

    SCB_EnableICache();
    SCB_EnableDCache();

    // A4/I-24: Per-timer timing computed below
    // R6-3: disarm prefill with conservative timing
    {
        dshot::DshotTimingConfig defaultTiming = dshot::calculateTiming(SystemCoreClock / 2, DSHOT_BITRATE_HZ);
        uint16_t disarmFrame = dshot::packThrottleFrame(0, false);
        for (int i = 0; i < NUM_MOTORS; i++)
            dshot::encodeToCompareBuffer(disarmFrame, defaultTiming, dshotCompareBuf[i]);
    }

    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    // ES0392 §2.5.4: Clear DMAMUX overrun flags before configuring
    DMAMUX1_ChannelStatus->CFR = 0xFFFFFFFF;

    numInitializedTimers = 0;

    for (int i = 0; i < NUM_MOTORS; i++) {
        const H7MotorRoute& r = motorRoutes[i];

        if (!timerAlreadyInitialized(r.timer)) {
            enableTimerClock(r.timer);
            hMotorTimer[i].Instance = r.timer;
            hMotorTimer[i].Init.Prescaler = 0;
            hMotorTimer[i].Init.CounterMode = TIM_COUNTERMODE_UP;
            hMotorTimer[i].Init.Period = dshotTiming.bitPeriodTicks - 1;
            hMotorTimer[i].Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
            hMotorTimer[i].Init.RepetitionCounter = 0;
            hMotorTimer[i].Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
            // F-27: HAL return check — partial motor init = do not fly
            if (HAL_TIM_PWM_Init(&hMotorTimer[i]) != HAL_OK) while(1) {}
            if (r.timer == TIM1 || r.timer == TIM8)
                __HAL_TIM_MOE_ENABLE(&hMotorTimer[i]);
            initializedTimers[numInitializedTimers++] = r.timer;
        }

        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;

        TIM_HandleTypeDef* htim = &hMotorTimer[i];
        for (int j = 0; j < i; j++)
            if (hMotorTimer[j].Instance == r.timer) { htim = &hMotorTimer[j]; break; }
        // F-27: HAL return check
        if (HAL_TIM_PWM_ConfigChannel(htim, &oc, r.channel) != HAL_OK) while(1) {}

        // G-4: GPIO init through wrapper (clock enable + DSB + init in one call)
        GPIO_InitTypeDef gpio = {};
        gpio.Pin = r.gpioPin;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = r.gpioAF;
        meltyGpioInit(r.gpioPort, &gpio);

        // H7 DMAMUX — programmable request routing
        // Use DMA1 streams 0-3 for motors (assigned sequentially)
        DMA_Stream_TypeDef* streams[] = { DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3 };
        hDmaMotor[i].Instance = streams[i];
        hDmaMotor[i].Init.Request = r.dmamuxRequest;
        hDmaMotor[i].Init.Direction = DMA_MEMORY_TO_PERIPH;
        hDmaMotor[i].Init.PeriphInc = DMA_PINC_DISABLE;
        hDmaMotor[i].Init.MemInc = DMA_MINC_ENABLE;
        hDmaMotor[i].Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hDmaMotor[i].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hDmaMotor[i].Init.Mode = DMA_NORMAL;
        hDmaMotor[i].Init.Priority = DMA_PRIORITY_HIGH;
        hDmaMotor[i].Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        // F-27: HAL return check
        if (HAL_DMA_Init(&hDmaMotor[i]) != HAL_OK) while(1) {}

        // A1/F-01: DMA linkage — HAL DMA IDs are CC1..CC4 = 1..4
        uint32_t dmaId = (r.channel >> 2) + 1;
        __HAL_LINKDMA(htim, hdma[dmaId], hDmaMotor[i]);
    }

    // A4/I-24: Per-timer-domain timing
    dshotTiming = dshot::calculateTiming(
        timerKernelClockHz(motorRoutes[0].timer), DSHOT_BITRATE_HZ);
}

// R13-1: Completion callback — same structure as F4/F7
static uint8_t routeBitForCompletion(TIM_TypeDef* instance, HAL_TIM_ActiveChannel activeChannel) {
    uint32_t completedChannel;
    switch (activeChannel) {
        case HAL_TIM_ACTIVE_CHANNEL_1: completedChannel = TIM_CHANNEL_1; break;
        case HAL_TIM_ACTIVE_CHANNEL_2: completedChannel = TIM_CHANNEL_2; break;
        case HAL_TIM_ACTIVE_CHANNEL_3: completedChannel = TIM_CHANNEL_3; break;
        case HAL_TIM_ACTIVE_CHANNEL_4: completedChannel = TIM_CHANNEL_4; break;
        default: return 0;
    }
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (motorRoutes[i].timer == instance && motorRoutes[i].channel == completedChannel)
            return (1 << i);
    }
    return 0;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* htim) {
    uint8_t bit = routeBitForCompletion(htim->Instance, htim->Channel);
    if (bit) dmaActiveMask &= ~bit;
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef* hdma) {
    dmaActiveMask = 0;
    (void)hdma;
}

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    if (motorIndex >= NUM_MOTORS) return;
    dshot::encodeToCompareBuffer(frame, dshotTiming, dshotCompareBuf[motorIndex]);
}

static TIM_HandleTypeDef* timerForMotor(int i) {
    TIM_HandleTypeDef* htim = &hMotorTimer[i];
    for (int j = 0; j < i; j++)
        if (hMotorTimer[j].Instance == motorRoutes[i].timer) { htim = &hMotorTimer[j]; break; }
    return htim;
}

static void stopActiveDmaStreams() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (dmaActiveMask & (1 << i))
            HAL_TIM_PWM_Stop_DMA(timerForMotor(i), motorRoutes[i].channel);
    }
    dmaActiveMask = 0;
}

bool dshotCommit(bool force) {
    if (dmaActiveMask != 0) {
        if (force) {
            stopActiveDmaStreams();
        } else {
            uint32_t now = HAL_GetTick();
            if ((now - dmaCommitTimestamp) > (DMA_TIMEOUT_US / 1000 + 1)) {
                stopActiveDmaStreams();
            } else {
                commitSkipCount++;
                return false;
            }
        }
    }
    dmaActiveMask = (1 << NUM_MOTORS) - 1;
    dmaCommitTimestamp = HAL_GetTick();
    for (int i = 0; i < NUM_MOTORS; i++) {
        HAL_TIM_PWM_Start_DMA(timerForMotor(i), motorRoutes[i].channel,
                              (uint32_t*)dshotCompareBuf[i], dshot::DSHOT_COMPARE_BUF_SIZE);
    }
    return true;
}

uint32_t dshotCommitSkips() { return commitSkipCount; }

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS) return 0;
    if (!telemReady[motorIndex]) return 0;
    telemReady[motorIndex] = false;
    return 0;
}

bool dshotTelemetryReady(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS) return false;
    return telemReady[motorIndex];
}

void dshotBidirTurnaround(uint8_t motorIndex, bool toInput) {
    if (motorIndex >= NUM_MOTORS) return;
    TIM_HandleTypeDef* htim = &hMotorTimer[motorIndex];
    for (int j = 0; j < motorIndex; j++)
        if (hMotorTimer[j].Instance == motorRoutes[motorIndex].timer) { htim = &hMotorTimer[j]; break; }

    if (toInput) {
        HAL_TIM_PWM_Stop_DMA(htim, motorRoutes[motorIndex].channel);
        TIM_IC_InitTypeDef ic = {};
        ic.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;
        ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
        ic.ICPrescaler = TIM_ICPSC_DIV1;
        ic.ICFilter = 0;
        HAL_TIM_IC_ConfigChannel(htim, &ic, motorRoutes[motorIndex].channel);
        HAL_TIM_IC_Start_DMA(htim, motorRoutes[motorIndex].channel, telemCaptureBuf[motorIndex], 32);
    } else {
        HAL_TIM_IC_Stop_DMA(htim, motorRoutes[motorIndex].channel);
        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(htim, &oc, motorRoutes[motorIndex].channel);
    }
}

} // namespace hal
} // namespace melty

#endif // STM32H7xx
#endif // NATIVE_BUILD
