// MeltyFC — DShot HAL: STM32F7 Family (F722, F745)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
// Uses DMA1/DMA2 stream+channel architecture (same as F4).
//
// KEY DIFFERENCE FROM F4:
//   F7 has L1 I/D caches. DMA buffers MUST live in DTCM (0x20000000-0x2000FFFF)
//   which is never cached AND DMA-accessible via AHBS slave port.
//   This solves cache coherency without any clean/invalidate calls.
//   See invariant I-11a, dma_buf.h, and PORTING.md.
//
// STUB — implementation BLOCKED on Step 0 hardware dump.
// Pin/timer/DMA assignments come from target pinmap.h.
// [2026-07-09] Created for T1 F722 HAL port.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32F7xx
#include "stm32f7xx_hal.h"

namespace melty {
namespace hal {

// DMA compare buffers — MUST be in DTCM (invariant I-11a)
static DMA_BUFFER_ATTR uint16_t dshotBuf[4][17]; // 4 motors x 17 compare values

// Bidir telemetry input-capture buffers — also DTCM
static DMA_BUFFER_ATTR uint32_t telemBuf[4][32]; // Raw capture timestamps
static volatile bool telemReady[4] = {false, false, false, false};

void dshotInit() {
    // Verify DMA buffers are in DTCM
    for (int i = 0; i < 4; i++) {
        DMA_BUFFER_ASSERT(dshotBuf[i]);
        DMA_BUFFER_ASSERT(telemBuf[i]);
    }

    // Enable L1 I-cache and D-cache (done once at boot, before DMA setup)
    // DTCM is never cached regardless — but the rest of the app benefits.
    SCB_EnableICache();
    SCB_EnableDCache();

    // TODO P2: Configure TIM8 (Aikon) or target-specific timer from pinmap.h
    // TODO P2: Configure DMA streams for each motor channel
    // TODO P2: Configure GPIO AF for motor pins
    // TODO P2: Configure input-capture for bidir telemetry
}

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    if (motorIndex >= 4)
        return;
    (void)frame;
    // TODO P2: Encode frame to dshotBuf[motorIndex] compare buffer
    // Same encoding as F4 — the pure-logic encodeToCompareBuffer() is shared.
    // Only the DMA stream assignment differs.
}

void dshotCommit() {
    // TODO P2: Trigger DMA for all motors
    // F7 DMA is structurally identical to F4 — same HAL_DMA_Start_IT() calls,
    // different stream/channel constants from pinmap.h.
}

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    if (motorIndex >= 4)
        return 0;
    if (!telemReady[motorIndex])
        return 0;
    telemReady[motorIndex] = false;
    // TODO P2: Decode capture timestamps to 21-bit GCR frame
    // No cache maintenance needed — DTCM is never cached.
    return 0;
}

bool dshotTelemetryReady(uint8_t motorIndex) {
    if (motorIndex >= 4)
        return false;
    return telemReady[motorIndex];
}

void dshotBidirTurnaround(uint8_t motorIndex, bool toInput) {
    if (motorIndex >= 4)
        return;
    (void)toInput;
    // TODO P2: Reconfigure GPIO direction for bidir turnaround
    // Same MODER register manipulation as F4, different GPIO port/pin from pinmap.h.
}

} // namespace hal
} // namespace melty

#endif // STM32F7xx
#endif // NATIVE_BUILD
