// MeltyFC — DShot HAL: STM32F4 Family (F405)
// Timer+DMA driver for DShot300 with bidirectional GCR telemetry.
// Uses DMA1/DMA2 stream+channel architecture (fixed mapping).
//
// STUB — implementation BLOCKED on Step 0 hardware dump.
// Pin/timer/DMA assignments come from target pinmap.h.

#ifndef NATIVE_BUILD

#include "hal/common/dshot_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"

namespace melty {
namespace hal {

// DMA compare buffers — must NOT be in CCM RAM
static DMA_BUFFER_ATTR uint16_t dshotBuf[4][17]; // 4 motors x 17 compare values

void dshotInit() {
    // Verify DMA buffers are NOT in CCM
    for (int i = 0; i < 4; i++) {
        DMA_BUFFER_ASSERT(dshotBuf[i]);
    }
    // TODO P2: Configure timer + DMA channels from pinmap.h
    // TODO P2: Configure GPIO alternate function for motor pins
}

void dshotSend(uint8_t motorIndex, uint16_t frame) {
    (void)motorIndex;
    (void)frame;
    // TODO P2: Encode frame to compare buffer, queue DMA
}

void dshotCommit() {
    // TODO P2: Trigger DMA for all motors
}

uint32_t dshotReadTelemetryRaw(uint8_t motorIndex) {
    (void)motorIndex;
    // TODO P2: Read input-capture DMA buffer, return raw GCR frame
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
}

} // namespace hal
} // namespace melty

#endif // STM32F4xx
#endif // NATIVE_BUILD
