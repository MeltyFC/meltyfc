// MeltyFC — DShot HAL: STM32H7 Family (H743)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
//
// H7 KEY DIFFERENCES (every trap has a ruling):
//   1. DMA buffers in D2 SRAM (0x30000000) — DTCM is CPU-private! (I-11b)
//   2. MPU non-cacheable region over .d2_dma — no cache maintenance in hot paths
//   3. DMAMUX for programmable DMA request routing (nice, not a trap)
//   4. Timer clock: 200MHz (AHB at 200MHz, APB2 timers get 200MHz)
//
// Pin/timer mapping from target pinmap.h (BF-derived + ArduPilot cross-ref):
//   MicoAir H743 V2: TIM1 CH1-CH4 on PE9/PE11/PE13/PE14, DMAMUX routed
//
// [2026-07-09] G2: Driver body written. MPU/PWR not hardware-blocked.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"
#include "dshot_common.hpp"

#ifdef STM32H7xx
#include "stm32h7xx_hal.h"
#include "target.h"

// MPU init from system_h7.cpp — call before any DMA buffer use
extern void meltyH7MpuInit(void);

namespace melty {
namespace hal {

static constexpr uint8_t NUM_MOTORS = 4;

// DShot300 timing — computed at init from SystemCoreClock (I-12)
static dshot::DshotTimingConfig dshotTiming;

// ============================================================================
// DMA buffers — MUST be in D2 SRAM (invariant I-11b)
// MPU non-cacheable region eliminates all cache maintenance
// ============================================================================

static DMA_BUFFER_ATTR uint16_t dshotCompareBuf[NUM_MOTORS][dshot::DSHOT_COMPARE_BUF_SIZE];
static DMA_BUFFER_ATTR uint32_t telemCaptureBuf[NUM_MOTORS][32];
static volatile bool telemReady[NUM_MOTORS] = {};
static volatile bool txInProgress = false;

static TIM_HandleTypeDef hMotorTimer;
static DMA_HandleTypeDef hDmaMotor[NUM_MOTORS];

static constexpr uint32_t motorChannel[NUM_MOTORS] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

// DMAMUX request IDs — use HAL-defined macros (transcription-proof)
static constexpr uint32_t dmamuxRequest[NUM_MOTORS] = {
    DMA_REQUEST_TIM1_CH1,  // 11
    DMA_REQUEST_TIM1_CH2,  // 12
    DMA_REQUEST_TIM1_CH3,  // 13
    DMA_REQUEST_TIM1_CH4,  // 14
};

// ============================================================================
// Init
// ============================================================================

void dshotInit() {
    // MPU non-cacheable region MUST be configured before any DMA buffer use
    // This is done once — system_h7.cpp::meltyH7MpuInit() configures MPU region 0
    // over D2 SRAM as Normal/Non-cacheable/Shareable
    meltyH7MpuInit();

    // Verify DMA buffers are in D2 SRAM (not DTCM!)
    for (int i = 0; i < NUM_MOTORS; i++) {
        DMA_BUFFER_ASSERT(dshotCompareBuf[i]);
        DMA_BUFFER_ASSERT(telemCaptureBuf[i]);
    }

    // I-12: Derive timing from SystemCoreClock
    // H7: APB2 timer clock = AHB clock = SystemCoreClock / 2 (at SYSCLK=400, AHB=200)
    uint32_t timerClock = SystemCoreClock / 2;
    dshotTiming = dshot::calculateTiming(timerClock, DSHOT_BITRATE_HZ);

    // R6-3: Prefill with encoded disarm frame (prevents random throttle at power-on)
    {
        uint16_t disarmFrame = dshot::packThrottleFrame(0, false);
        for (int i = 0; i < NUM_MOTORS; i++) {
            dshot::encodeToCompareBuffer(disarmFrame, dshotTiming, dshotCompareBuf[i]);
        }
    }

    // Enable caches (D-cache safe — MPU region protects DMA buffers)
    SCB_EnableICache();
    SCB_EnableDCache();

    // Enable clocks
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    __HAL_RCC_TIM1_CLK_ENABLE();

    // Configure timer
    hMotorTimer.Instance = MOTOR1_TIMER;
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

    // Configure DMA with DMAMUX routing
    // H7 DMAMUX: any DMA stream can serve any peripheral request
    for (int i = 0; i < NUM_MOTORS; i++) {
        hDmaMotor[i].Instance = DMA1_Stream0 + i;  // Use DMA1 streams 0-3
        hDmaMotor[i].Init.Request = dmamuxRequest[i];
        hDmaMotor[i].Init.Direction = DMA_MEMORY_TO_PERIPH;
        hDmaMotor[i].Init.PeriphInc = DMA_PINC_DISABLE;
        hDmaMotor[i].Init.MemInc = DMA_MINC_ENABLE;
        hDmaMotor[i].Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hDmaMotor[i].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hDmaMotor[i].Init.Mode = DMA_NORMAL;
        hDmaMotor[i].Init.Priority = DMA_PRIORITY_HIGH;
        hDmaMotor[i].Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        HAL_DMA_Init(&hDmaMotor[i]);

        // Link DMA to timer channel
        __HAL_LINKDMA(&hMotorTimer, hdma[motorChannel[i] >> 2], hDmaMotor[i]);
    }

    __HAL_TIM_MOE_ENABLE(&hMotorTimer);
}

// ============================================================================
// Send / Commit
// ============================================================================

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    if (motorIndex >= NUM_MOTORS)
        return;
    // No cache clean needed — MPU non-cacheable region handles it
    dshot::encodeToCompareBuffer(frame, dshotTiming, dshotCompareBuf[motorIndex]);
}

void dshotCommit() {
    if (txInProgress)
        return;
    txInProgress = true;

    // No cache clean needed before DMA read — MPU non-cacheable region
    for (int i = 0; i < NUM_MOTORS; i++) {
        HAL_TIM_PWM_Start_DMA(&hMotorTimer, motorChannel[i],
                              (uint32_t*)dshotCompareBuf[i],
                              dshot::DSHOT_COMPARE_BUF_SIZE);
    }
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
    // No cache invalidate needed — MPU non-cacheable region
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

    // CRITICAL: no cache maintenance here — 30us turnaround window
    // MPU non-cacheable region is the ONLY acceptable solution
    if (toInput) {
        HAL_TIM_PWM_Stop_DMA(&hMotorTimer, motorChannel[motorIndex]);
        TIM_IC_InitTypeDef ic = {};
        ic.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;
        ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
        ic.ICPrescaler = TIM_ICPSC_DIV1;
        ic.ICFilter = 0;
        HAL_TIM_IC_ConfigChannel(&hMotorTimer, &ic, motorChannel[motorIndex]);
        // No cache invalidate before DMA write to capture buffer — MPU handles it
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

#endif // STM32H7xx
#endif // NATIVE_BUILD
