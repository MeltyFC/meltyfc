// MeltyFC — DShot HAL: STM32H7 Family (H743)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
// Uses DMAMUX for programmable request routing (unlike F4/F7 fixed mapping).
//
// H7 TRAPS addressed:
//   - DMA buffers in D2 SRAM (0x30000000), NOT DTCM (CPU-private on H7!)
//   - MPU non-cacheable region over .d2_dma section — no cache maintenance calls
//   - DMAMUX assignment layer for timer→DMA request routing
//   - D-cache invalidation REJECTED for hot paths (30us bidir turnaround)
//
// STUB — implementation BLOCKED on Step 0 hardware dump.
// Pin/timer/DMA assignments come from target pinmap.h.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32H7xx
#include "stm32h7xx_hal.h"

namespace melty {
namespace hal {

// DMA compare buffers — MUST be in D2 SRAM (invariant I-11b)
// MPU non-cacheable region eliminates clean/invalidate in bidir turnaround
static DMA_BUFFER_ATTR uint16_t dshotBuf[4][17]; // 4 motors x 17 compare values

// Bidir telemetry capture buffers — also in D2 SRAM
static DMA_BUFFER_ATTR uint32_t dshotCapture[4][32]; // input capture for GCR decode

void dshotInit() {
    // Verify DMA buffers are in D2 SRAM — not DTCM!
    for (int i = 0; i < 4; i++) {
        DMA_BUFFER_ASSERT(dshotBuf[i]);
        DMA_BUFFER_ASSERT(dshotCapture[i]);
    }

    // TODO P2: Configure MPU region for .d2_dma section
    //   - Region base: 0x30000000
    //   - Size: cover .d2_dma extent
    //   - Attributes: Normal, Non-cacheable, Shareable
    //   - This is configured ONCE at init, never touched again
    //   MPU_Region_InitTypeDef mpu_init;
    //   mpu_init.Enable = MPU_REGION_ENABLE;
    //   mpu_init.BaseAddress = 0x30000000;
    //   mpu_init.Size = MPU_REGION_SIZE_32KB;
    //   mpu_init.AccessPermission = MPU_REGION_FULL_ACCESS;
    //   mpu_init.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    //   mpu_init.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    //   mpu_init.IsShareable = MPU_ACCESS_SHAREABLE;
    //   mpu_init.TypeExtField = MPU_TEX_LEVEL1;
    //   mpu_init.SubRegionDisable = 0;
    //   mpu_init.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    //   HAL_MPU_ConfigRegion(&mpu_init);

    // TODO P2: Configure DMAMUX for motor timer channels
    //   H7 DMAMUX is programmable — no fixed stream+channel mapping
    //   Route timer update/CC events to DMA streams via DMAMUX request IDs

    // TODO P2: Configure timer + GPIO AF for motor pins from pinmap.h
}

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    (void)motorIndex;
    (void)frame;
    // TODO P2: Encode frame to compare buffer, queue DMA via DMAMUX
}

void dshotCommit() {
    // TODO P2: Trigger DMA for all motors
    // No cache clean needed — MPU non-cacheable region handles it
}

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    (void)motorIndex;
    // TODO P2: Read input-capture DMA buffer from D2 SRAM, return raw GCR frame
    // No cache invalidate needed — MPU non-cacheable region handles it
    return 0;
}

bool dshotTelemetryReady(uint8_t motorIndex) {
    (void)motorIndex;
    return false;
}

void dshotBidirTurnaround(uint8_t motorIndex, bool toInput) {
    (void)motorIndex;
    (void)toInput;
    // TODO P2: Reconfigure GPIO direction for bidir turnaround
    // CRITICAL: no cache maintenance here — 30us turnaround window
    // MPU non-cacheable region is the ONLY acceptable solution
}

} // namespace hal
} // namespace melty

#endif // STM32H7xx
#endif // NATIVE_BUILD
