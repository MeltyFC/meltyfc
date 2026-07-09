// MeltyFC — DShot HAL: STM32F4 Family (F405, F411)
// A1: Table-driven timer/DMA routing — no hardcoded timer instances.
// Each motor's timer/channel/AF/DMA comes from the target's pinmap.h.
// Supports both single-timer (TIM1×4) and split-timer (TIM3+TIM4) boards.
//
// [2026-07-09] A1 refactor: route table replaces hardcoded TIM1 assumption.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "dshot_common.hpp"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

static constexpr uint8_t NUM_MOTORS = 4;

// DShot300 timing — computed at init from SystemCoreClock (I-12 + I-16)
static dshot::DshotTimingConfig dshotTiming;

// DMA buffers — must NOT be in CCM (invariant I-11)
static DMA_BUFFER_ATTR uint16_t dshotCompareBuf[NUM_MOTORS][dshot::DSHOT_COMPARE_BUF_SIZE];
static DMA_BUFFER_ATTR uint32_t telemCaptureBuf[NUM_MOTORS][32];
static volatile bool telemReady[NUM_MOTORS] = {};
static volatile bool txInProgress = false;

// A1: Per-motor timer handle array — supports multi-timer boards
// Deduplicated: if all motors share one timer, only one handle is initialized
static TIM_HandleTypeDef hMotorTimer[NUM_MOTORS];
static DMA_HandleTypeDef hDmaMotor[NUM_MOTORS];

// A1: Route table — populated from pinmap defines
struct F4MotorRoute {
    TIM_TypeDef* timer;
    uint32_t channel;
    GPIO_TypeDef* gpioPort;
    uint16_t gpioPin;
    uint8_t gpioAF;
    DMA_Stream_TypeDef* dmaStream;
    uint32_t dmaChannel;
};

static const F4MotorRoute motorRoutes[NUM_MOTORS] = {
    { MOTOR1_TIMER, MOTOR1_CHANNEL, MOTOR1_GPIO_PORT, MOTOR1_GPIO_PIN, MOTOR1_AF, MOTOR1_DMA_STREAM, MOTOR1_DMA_CHANNEL },
    { MOTOR2_TIMER, MOTOR2_CHANNEL, MOTOR2_GPIO_PORT, MOTOR2_GPIO_PIN, MOTOR2_AF, MOTOR2_DMA_STREAM, MOTOR2_DMA_CHANNEL },
    { MOTOR3_TIMER, MOTOR3_CHANNEL, MOTOR3_GPIO_PORT, MOTOR3_GPIO_PIN, MOTOR3_AF, MOTOR3_DMA_STREAM, MOTOR3_DMA_CHANNEL },
    { MOTOR4_TIMER, MOTOR4_CHANNEL, MOTOR4_GPIO_PORT, MOTOR4_GPIO_PIN, MOTOR4_AF, MOTOR4_DMA_STREAM, MOTOR4_DMA_CHANNEL },
};

// Track which timers we've already initialized (for split-timer dedup)
static TIM_TypeDef* initializedTimers[NUM_MOTORS] = {};
static uint8_t numInitializedTimers = 0;

static bool timerAlreadyInitialized(TIM_TypeDef* tim) {
    for (uint8_t i = 0; i < numInitializedTimers; i++) {
        if (initializedTimers[i] == tim) return true;
    }
    return false;
}

static void enableTimerClock(TIM_TypeDef* tim) {
    // Enable the RCC clock for any timer we might use
    if (tim == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (tim == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (tim == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
#ifdef TIM8
    else if (tim == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
#endif
}

// ============================================================================
// Init
// ============================================================================

void dshotInit() {
    // Verify DMA buffers are NOT in CCM
    for (int i = 0; i < NUM_MOTORS; i++) {
        DMA_BUFFER_ASSERT(dshotCompareBuf[i]);
        DMA_BUFFER_ASSERT(telemCaptureBuf[i]);
    }

    // I-12 + I-16: Derive timing from SystemCoreClock
    // F4: timer clock = SystemCoreClock (APB2 prescaler !=1, ×2 multiplier)
    uint32_t timerClock = SystemCoreClock;

#ifdef EXPECTED_TIMER_CLOCK_HZ
    if (timerClock < (EXPECTED_TIMER_CLOCK_HZ * 95 / 100) ||
        timerClock > (EXPECTED_TIMER_CLOCK_HZ * 105 / 100)) {
        while (1) {} // I-16: Timer clock mismatch
    }
#endif

    dshotTiming = dshot::calculateTiming(timerClock, DSHOT_BITRATE_HZ);

    // R6-3: Prefill with encoded disarm frame
    {
        uint16_t disarmFrame = dshot::packThrottleFrame(0, false);
        for (int i = 0; i < NUM_MOTORS; i++) {
            dshot::encodeToCompareBuffer(disarmFrame, dshotTiming, dshotCompareBuf[i]);
        }
    }

    // Enable DMA clocks
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    // A1: Table-driven timer + GPIO + DMA init
    numInitializedTimers = 0;

    for (int i = 0; i < NUM_MOTORS; i++) {
        const F4MotorRoute& route = motorRoutes[i];

        // Enable timer clock (once per unique timer)
        if (!timerAlreadyInitialized(route.timer)) {
            enableTimerClock(route.timer);

            // Configure timer base
            hMotorTimer[i].Instance = route.timer;
            hMotorTimer[i].Init.Prescaler = 0;
            hMotorTimer[i].Init.CounterMode = TIM_COUNTERMODE_UP;
            hMotorTimer[i].Init.Period = dshotTiming.bitPeriodTicks - 1;
            hMotorTimer[i].Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
            hMotorTimer[i].Init.RepetitionCounter = 0;
            hMotorTimer[i].Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
            HAL_TIM_PWM_Init(&hMotorTimer[i]);

            // MOE only for TIM1/TIM8 (ES0182 §2.6.1: break not connected, safe)
            if (route.timer == TIM1
#ifdef TIM8
                || route.timer == TIM8
#endif
            ) {
                __HAL_TIM_MOE_ENABLE(&hMotorTimer[i]);
            }

            initializedTimers[numInitializedTimers++] = route.timer;
        }

        // Configure output channel
        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
        oc.OCIdleState = TIM_OCIDLESTATE_RESET;

        // Find the timer handle for this motor's timer
        TIM_HandleTypeDef* htim = &hMotorTimer[i];
        for (int j = 0; j < i; j++) {
            if (hMotorTimer[j].Instance == route.timer) {
                htim = &hMotorTimer[j];
                break;
            }
        }
        HAL_TIM_PWM_ConfigChannel(htim, &oc, route.channel);

        // Configure GPIO with correct AF
        GPIO_InitTypeDef gpio = {};
        gpio.Pin = route.gpioPin;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = route.gpioAF;
        HAL_GPIO_Init(route.gpioPort, &gpio);

        // DMA configuration — I-3: explicit DMA_NORMAL
        hDmaMotor[i].Instance = route.dmaStream;
        hDmaMotor[i].Init.Channel = route.dmaChannel;
        hDmaMotor[i].Init.Direction = DMA_MEMORY_TO_PERIPH;
        hDmaMotor[i].Init.PeriphInc = DMA_PINC_DISABLE;
        hDmaMotor[i].Init.MemInc = DMA_MINC_ENABLE;
        hDmaMotor[i].Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hDmaMotor[i].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hDmaMotor[i].Init.Mode = DMA_NORMAL;  // I-3: never circular
        hDmaMotor[i].Init.Priority = DMA_PRIORITY_HIGH;
        hDmaMotor[i].Init.FIFOMode = DMA_FIFOMODE_DISABLE;  // E-03: direct mode
        HAL_DMA_Init(&hDmaMotor[i]);
    }
}

// ============================================================================
// Send / Commit
// ============================================================================

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    if (motorIndex >= NUM_MOTORS)
        return;
    dshot::encodeToCompareBuffer(frame, dshotTiming, dshotCompareBuf[motorIndex]);
}

void dshotCommit() {
    if (txInProgress)
        return;
    txInProgress = true;

    for (int i = 0; i < NUM_MOTORS; i++) {
        // Find the right timer handle for this motor
        TIM_HandleTypeDef* htim = &hMotorTimer[i];
        for (int j = 0; j < i; j++) {
            if (hMotorTimer[j].Instance == motorRoutes[i].timer) {
                htim = &hMotorTimer[j];
                break;
            }
        }
        HAL_TIM_PWM_Start_DMA(htim, motorRoutes[i].channel,
                              (uint32_t*)dshotCompareBuf[i],
                              dshot::DSHOT_COMPARE_BUF_SIZE);
    }
}

// ============================================================================
// Bidirectional telemetry
// ============================================================================

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS) return 0;
    if (!telemReady[motorIndex]) return 0;
    telemReady[motorIndex] = false;
    return 0; // TODO: edge-to-GCR conversion
}

bool dshotTelemetryReady(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS) return false;
    return telemReady[motorIndex];
}

void dshotBidirTurnaround(uint8_t motorIndex, bool toInput) {
    if (motorIndex >= NUM_MOTORS) return;

    TIM_HandleTypeDef* htim = &hMotorTimer[motorIndex];
    for (int j = 0; j < motorIndex; j++) {
        if (hMotorTimer[j].Instance == motorRoutes[motorIndex].timer) {
            htim = &hMotorTimer[j];
            break;
        }
    }

    if (toInput) {
        HAL_TIM_PWM_Stop_DMA(htim, motorRoutes[motorIndex].channel);
        TIM_IC_InitTypeDef ic = {};
        ic.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;
        ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
        ic.ICPrescaler = TIM_ICPSC_DIV1;
        ic.ICFilter = 0;
        HAL_TIM_IC_ConfigChannel(htim, &ic, motorRoutes[motorIndex].channel);
        HAL_TIM_IC_Start_DMA(htim, motorRoutes[motorIndex].channel,
                             telemCaptureBuf[motorIndex], 32);
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

#endif // STM32F4xx
#endif // NATIVE_BUILD
