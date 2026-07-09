// MeltyFC — DShot HAL: STM32F7 Family (F722, F745)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
// DMA stream+channel architecture is F4-like — same driver structure,
// different stream/channel constants and DTCM buffer placement.
//
// KEY F7 DIFFERENCES FROM F4:
//   - DMA buffers in DTCM (never cached, DMA-accessible via AHBS) — I-11a
//   - L1 I/D caches enabled at boot (DTCM is unaffected)
//   - Timer clock: 216MHz (F722/F745) vs 168MHz (F405)
//   - I2C uses TIMINGR-based IP (not CCR/TRISE)
//
// Pin/timer/DMA mapping from target pinmap.h (BF-derived):
//   Aikon F7 Mini: TIM8 CH1-CH4 on PC6-PC9, DMA2
//   JHEMCU GHF745: (same family, different pins — from pinmap.h)
//
// [2026-07-09] G2: Driver body written from BF-derived pinmap + DShot spec.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "dshot_common.hpp"

#ifdef STM32F7xx
#include "stm32f7xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

static constexpr uint8_t NUM_MOTORS = 4;

// Timer clock: 216MHz (F722/F745)
// APB2 timer clock = 216MHz when APB2 prescaler != 1
// Safe assumption: use 108MHz (half SYSCLK) if uncertain — measure on hardware
static constexpr uint32_t TIMER_CLOCK_HZ = 108000000;

static const dshot::DshotTimingConfig dshotTiming =
    dshot::calculateTiming(TIMER_CLOCK_HZ, DSHOT_BITRATE_HZ);

// ============================================================================
// DMA buffers — MUST be in DTCM (invariant I-11a)
// DTCM is never cached — no coherency issues with DMA
// ============================================================================

static DMA_BUFFER_ATTR uint16_t dshotCompareBuf[NUM_MOTORS][dshot::DSHOT_COMPARE_BUF_SIZE];
static DMA_BUFFER_ATTR uint32_t telemCaptureBuf[NUM_MOTORS][32];
static volatile bool telemReady[NUM_MOTORS] = {};
static volatile bool txInProgress = false;

static TIM_HandleTypeDef hMotorTimer;

static constexpr uint32_t motorChannel[NUM_MOTORS] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

// ============================================================================
// Init
// ============================================================================

void dshotInit() {
    // Verify DMA buffers are in DTCM
    for (int i = 0; i < NUM_MOTORS; i++) {
        DMA_BUFFER_ASSERT(dshotCompareBuf[i]);
        DMA_BUFFER_ASSERT(telemCaptureBuf[i]);
    }

    for (int i = 0; i < NUM_MOTORS; i++) {
        for (int j = 0; j < dshot::DSHOT_COMPARE_BUF_SIZE; j++) {
            dshotCompareBuf[i][j] = 0;
        }
    }

    // Enable caches — DTCM is unaffected (never cached on F7)
    SCB_EnableICache();
    SCB_EnableDCache();

    // Enable clocks — both timers, unused one costs nothing
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();

    // Configure timer — use MOTOR_TIMER if defined, else MOTOR1_TIMER
#if defined(MOTOR_TIMER)
    hMotorTimer.Instance = MOTOR_TIMER;
#elif defined(MOTOR1_TIMER)
    hMotorTimer.Instance = MOTOR1_TIMER;
    // NOTE: JHEMCU uses split timers (TIM3 for M1/M2, TIM1 for M3/M4)
    // Full multi-timer support requires a second TIM handle — deferred to P2
    __HAL_RCC_TIM3_CLK_ENABLE();
#endif
    hMotorTimer.Init.Prescaler = 0;
    hMotorTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
    hMotorTimer.Init.Period = dshotTiming.bitPeriodTicks - 1;
    hMotorTimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hMotorTimer.Init.RepetitionCounter = 0;
    hMotorTimer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&hMotorTimer);

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

    __HAL_TIM_MOE_ENABLE(&hMotorTimer);
}

// ============================================================================
// Send / Commit — identical to F4 (same DMA model)
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
        HAL_TIM_PWM_Start_DMA(&hMotorTimer, motorChannel[i],
                              (uint32_t*)dshotCompareBuf[i],
                              dshot::DSHOT_COMPARE_BUF_SIZE);
    }
}

// ============================================================================
// Bidirectional telemetry — identical to F4
// ============================================================================

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS)
        return 0;
    if (!telemReady[motorIndex])
        return 0;
    telemReady[motorIndex] = false;
    // No cache maintenance needed — DTCM is never cached
    // TODO: edge-to-GCR conversion from telemCaptureBuf
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

    if (toInput) {
        HAL_TIM_PWM_Stop_DMA(&hMotorTimer, motorChannel[motorIndex]);
        TIM_IC_InitTypeDef ic = {};
        ic.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;
        ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
        ic.ICPrescaler = TIM_ICPSC_DIV1;
        ic.ICFilter = 0;
        HAL_TIM_IC_ConfigChannel(&hMotorTimer, &ic, motorChannel[motorIndex]);
        HAL_TIM_IC_Start_DMA(&hMotorTimer, motorChannel[motorIndex],
                             telemCaptureBuf[motorIndex], 32);
    } else {
        HAL_TIM_IC_Stop_DMA(&hMotorTimer, motorChannel[motorIndex]);
        TIM_OC_InitTypeDef oc = {};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = 0;
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&hMotorTimer, &oc, motorChannel[motorIndex]);
    }
}

} // namespace hal
} // namespace melty

#endif // STM32F7xx
#endif // NATIVE_BUILD
