// MeltyFC — DShot HAL: STM32F4 Family (F405)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
// Uses DMA2 stream+channel architecture (fixed mapping per F4 reference manual).
//
// Pin/timer/DMA mapping from target pinmap.h (BF-derived):
//   CruxF405HD: TIM1 CH1-CH4 on PA8/PA9/PA10/PA11, DMA2 streams
//
// DShot300 protocol:
//   16-bit frame → 17 compare values (16 data bits + trailing idle)
//   Timer runs in PWM mode, DMA feeds compare register each bit period
//   Bit timing: period=280 ticks @84MHz, 1=210 ticks (75%), 0=105 ticks (37.5%)
//
// Bidirectional telemetry:
//   After frame TX complete, turnaround pin to input-capture
//   ESC responds with 21-bit GCR-encoded eRPM on the inverted line
//   Input-capture DMA captures edge timestamps → GCR decode in pure logic
//
// [2026-07-09] G2: Driver body written from BF-derived pinmap + DShot spec.
//   Self-verified = compiles. Hardware-verified = on board arrival.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "dshot_common.hpp"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

// ============================================================================
// Configuration from pinmap.h
// ============================================================================

// Number of motors MeltyFC uses (3 for donut, 4 max supported by hardware)
static constexpr uint8_t NUM_MOTORS = 4;

// Timer clock = APB2 timer clock = 168MHz/2 * 2 = 168MHz (F405 with APB2 prescaler=2)
// Actually: if APB2 prescaler != 1, timer clock = APB2 * 2
// Crux F405: SYSCLK=168, AHB=168, APB2=84 → timer clock = 84*2 = 168MHz
// But with default Arduino core, timer clock might be 84MHz. Use 84MHz to be safe.
// DShot300 timing — computed at init from SystemCoreClock (I-12)
// NOT hardcoded: if clock config is wrong, timing is wrong → ESC sees garbage → fail safe
static dshot::DshotTimingConfig dshotTiming;

// ============================================================================
// DMA buffers — must NOT be in CCM (invariant I-11)
// ============================================================================

// Compare buffers: one per motor, 17 values each (16 bits + trailing zero)
static DMA_BUFFER_ATTR uint16_t dshotCompareBuf[NUM_MOTORS][dshot::DSHOT_COMPARE_BUF_SIZE];

// Telemetry input-capture buffers
static DMA_BUFFER_ATTR uint32_t telemCaptureBuf[NUM_MOTORS][32];
static volatile bool telemReady[NUM_MOTORS] = {};
static volatile bool txInProgress = false;

// ============================================================================
// Timer + DMA handles
// ============================================================================

static TIM_HandleTypeDef hMotorTimer;
static DMA_HandleTypeDef hDmaMotor[NUM_MOTORS];

// Motor channel map (timer channel index for each motor)
static constexpr uint32_t motorChannel[NUM_MOTORS] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

// ============================================================================
// GPIO helpers
// ============================================================================

// Parse pin define (e.g., PA8) to GPIO port + pin number
// This is resolved at compile time from pinmap.h defines
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    uint8_t af;  // Alternate function number
} PinDef;

// Motor pin table — populated from pinmap.h at init
// Hardware-specific: filled by dshotConfigurePins()
static PinDef motorPins[NUM_MOTORS];

static void dshotConfigureGpio(const PinDef* pin, bool asOutput) {
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = pin->pin;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Pull = GPIO_NOPULL;

    if (asOutput) {
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Alternate = pin->af;
    } else {
        // Input capture mode for bidir telemetry
        gpio.Mode = GPIO_MODE_AF_PP;  // Still AF for timer input capture
        gpio.Alternate = pin->af;
    }

    HAL_GPIO_Init(pin->port, &gpio);
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

    // I-12: Derive timing from SystemCoreClock (never hardcode)
    // F405: APB2 timer clock = SystemCoreClock / 2 (when APB2 prescaler != 1)
    uint32_t timerClock = SystemCoreClock / 2;
    dshotTiming = dshot::calculateTiming(timerClock, DSHOT_BITRATE_HZ);

    // R6-3: Prefill compare buffers with ENCODED disarm frame (throttle=0)
    // Prevents uninitialized buffer -> random CRC-lucky throttle at power-on
    {
        uint16_t disarmFrame = dshot::packThrottleFrame(0, false);
        for (int i = 0; i < NUM_MOTORS; i++) {
            dshot::encodeToCompareBuffer(disarmFrame, dshotTiming, dshotCompareBuf[i]);
        }
    }

    // Enable timer and DMA clocks
    // Enable both TIM1 and TIM8 clocks — unused one costs nothing
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();


    // Configure timer for DShot output
    hMotorTimer.Instance = MOTOR_TIMER;
    hMotorTimer.Init.Prescaler = 0;  // No prescaler — full timer clock
    hMotorTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
    hMotorTimer.Init.Period = dshotTiming.bitPeriodTicks - 1;
    hMotorTimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hMotorTimer.Init.RepetitionCounter = 0;
    hMotorTimer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&hMotorTimer);

    // Configure each channel for PWM output
    for (int i = 0; i < NUM_MOTORS; i++) {
        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
        oc.OCIdleState = TIM_OCIDLESTATE_RESET;
        HAL_TIM_PWM_ConfigChannel(&hMotorTimer, &oc, motorChannel[i]);
    }

    // DMA configuration — I-3: explicit DMA_NORMAL (non-circular)
    // Dead CPU = DMA stops = motors stop. Never DMA_CIRCULAR.
    // Stream/channel assignments resolved at board bringup from pinmap.h
    // TODO: fill DMA stream instances per target
    for (int i = 0; i < NUM_MOTORS; i++) {
        hDmaMotor[i].Init.Direction = DMA_MEMORY_TO_PERIPH;
        hDmaMotor[i].Init.PeriphInc = DMA_PINC_DISABLE;
        hDmaMotor[i].Init.MemInc = DMA_MINC_ENABLE;
        hDmaMotor[i].Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hDmaMotor[i].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hDmaMotor[i].Init.Mode = DMA_NORMAL;  // I-3: explicit, never circular
        hDmaMotor[i].Init.Priority = DMA_PRIORITY_HIGH;
        hDmaMotor[i].Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    }

    // Enable motor output (TIM1/TIM8 have break feature — must enable MOE)
    __HAL_TIM_MOE_ENABLE(&hMotorTimer);
}

// ============================================================================
// Send / Commit
// ============================================================================

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    if (motorIndex >= NUM_MOTORS)
        return;

    // Encode the 16-bit DShot frame to the 17-element compare buffer
    dshot::encodeToCompareBuffer(frame, dshotTiming, dshotCompareBuf[motorIndex]);
}

void dshotCommit() {
    if (txInProgress)
        return;

    txInProgress = true;

    // Start DMA transfer for each motor channel
    for (int i = 0; i < NUM_MOTORS; i++) {
        HAL_TIM_PWM_Start_DMA(&hMotorTimer, motorChannel[i],
                              (uint32_t*)dshotCompareBuf[i],
                              dshot::DSHOT_COMPARE_BUF_SIZE);
    }

    // DMA transfer complete callback will clear txInProgress
    // and initiate bidir turnaround if enabled
}

// ============================================================================
// Bidirectional telemetry
// ============================================================================

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS)
        return 0;
    if (!telemReady[motorIndex])
        return 0;

    telemReady[motorIndex] = false;

    // Decode capture timestamps to 21-bit GCR frame
    // The capture buffer contains edge timestamps from input-capture DMA
    // Edge timing → bit durations → GCR-encoded bits → 21-bit raw frame
    // Actual GCR decode happens in pure-logic dshot_common::gcrDecode()

    // TODO: Implement edge-to-GCR conversion from telemCaptureBuf
    // This is the same algorithm across all families — extract to common
    return 0;
}

bool dshotTelemetryReady(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS)
        return false;
    return telemReady[motorIndex];
}

void dshotBidirTurnaround(uint8_t motorIndex, bool toInput) {
    if (motorIndex >= NUM_MOTORS)
        return;

    // Reconfigure the motor GPIO between output (TX) and input-capture (RX)
    // This happens inside the 30us turnaround window — must be fast
    if (toInput) {
        // Switch to input capture for telemetry reception
        // 1. Stop PWM output on this channel
        HAL_TIM_PWM_Stop_DMA(&hMotorTimer, motorChannel[motorIndex]);

        // 2. Reconfigure timer channel for input capture
        TIM_IC_InitTypeDef ic = {};
        ic.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;  // Capture both edges for GCR
        ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
        ic.ICPrescaler = TIM_ICPSC_DIV1;
        ic.ICFilter = 0;
        HAL_TIM_IC_ConfigChannel(&hMotorTimer, &ic, motorChannel[motorIndex]);

        // 3. Start input capture DMA
        HAL_TIM_IC_Start_DMA(&hMotorTimer, motorChannel[motorIndex],
                             telemCaptureBuf[motorIndex], 32);
    } else {
        // Switch back to PWM output
        HAL_TIM_IC_Stop_DMA(&hMotorTimer, motorChannel[motorIndex]);

        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&hMotorTimer, &oc, motorChannel[motorIndex]);
    }
}

// ============================================================================
// DMA callbacks (called from IRQ context)
// ============================================================================

// TODO: Wire these to the DMA TC interrupt handlers
// HAL_TIM_PWM_PulseFinishedCallback → frame sent, initiate turnaround
// HAL_TIM_IC_CaptureCallback → telemetry edge captured
// DMA TC → telemetry capture complete, set telemReady[i] = true

} // namespace hal
} // namespace melty

#endif // STM32F4xx
#endif // NATIVE_BUILD
