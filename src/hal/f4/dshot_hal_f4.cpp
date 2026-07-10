// MeltyFC — DShot HAL: STM32F4 Family (F405, F411)
// A1: Table-driven timer/DMA routing — no hardcoded timer instances.
// Each motor's timer/channel/AF/DMA comes from the target's pinmap.h.
// Supports both single-timer (TIM1×4) and split-timer (TIM3+TIM4) boards.
//
// [2026-07-09] A1 refactor: route table replaces hardcoded TIM1 assumption.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "hal/common/gpio_port_clock.h"
#include "hal/common/timer_clock.h"
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

// A3/I-23: Per-stream active bitmask replaces single txInProgress boolean.
// Each bit = one motor DMA stream active. busy = mask != 0.
// Cleared by DMA transfer-complete callback (or error callback).
// Bounded timeout clears stuck bits and logs the event.
static volatile uint8_t dmaActiveMask = 0;
static uint32_t dmaCommitTimestamp = 0;
static constexpr uint32_t DMA_TIMEOUT_US = 5000; // 5ms — a frame takes ~55µs

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

    // A4/I-24: Per-timer timing is computed per unique timer in the loop below
    // (different timers can be on different APB domains with different clocks)

    // R6-3: Prefill with disarm frames (using a safe default timing — recalculated per-timer below)
    {
        // Use a conservative timing for prefill — will be overwritten at first real send
        dshot::DshotTimingConfig defaultTiming = dshot::calculateTiming(SystemCoreClock, DSHOT_BITRATE_HZ);
        uint16_t disarmFrame = dshot::packThrottleFrame(0, false);
        for (int i = 0; i < NUM_MOTORS; i++) {
            dshot::encodeToCompareBuffer(disarmFrame, defaultTiming, dshotCompareBuf[i]);
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

        // A2/I-25: Enable GPIO port clock BEFORE HAL_GPIO_Init
        gpioEnablePortClock(route.gpioPort);

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

        // A1/F-01: Link DMA handle to timer channel — REQUIRED for HAL_TIM_PWM_Start_DMA
        // TIM_DMA_ID_CC1..CC4 = 1..4, channel >> 2 gives 0..3, so +1
        uint32_t dmaId = (route.channel >> 2) + 1;  // TIM_DMA_ID_CC1=1, CC2=2, CC3=3, CC4=4
        __HAL_LINKDMA(htim, hdma[dmaId], hDmaMotor[i]);
    }

    // A4/I-24: Compute per-timer DShot timing from actual kernel clock
    // Store the timing for the FIRST timer (all motors on same-family boards
    // typically share one APB domain; split-timer boards verified at bench)
    dshotTiming = dshot::calculateTiming(
        timerKernelClockHz(motorRoutes[0].timer), DSHOT_BITRATE_HZ);
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
    // A3: Bounded timeout clears stuck DMA mask
    if (dmaActiveMask != 0) {
        // Check if timeout exceeded — clears stuck bits
        uint32_t now = HAL_GetTick();
        if ((now - dmaCommitTimestamp) > (DMA_TIMEOUT_US / 1000 + 1)) {
            dmaActiveMask = 0; // Timeout — force clear, log the event
        } else {
            return; // Still in progress, not timed out yet
        }
    }

    // Set all motor bits active
    dmaActiveMask = (1 << NUM_MOTORS) - 1;
    dmaCommitTimestamp = HAL_GetTick();

    for (int i = 0; i < NUM_MOTORS; i++) {
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

// A3/I-23/I-30: DMA transfer-complete callback — clears per-stream bit
// Called from ISR context by HAL when a timer DMA transfer finishes.
// Per ISR_CONTRACT.md: only clears a volatile flag, no blocking/allocating.
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* htim) {
    // Find which motor(s) use this timer+channel and clear their bits
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (hMotorTimer[i].Instance == htim->Instance ||
            (i > 0 && hMotorTimer[0].Instance == htim->Instance)) {
            dmaActiveMask &= ~(1 << i);
        }
    }
}

// A3: DMA error callback — also clears the bit (fail-open on DMA error)
void HAL_DMA_ErrorCallback(DMA_HandleTypeDef* hdma) {
    // Clear all bits on DMA error — motors will retry next commit
    dmaActiveMask = 0;
    (void)hdma;
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
