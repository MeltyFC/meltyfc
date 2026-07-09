// MeltyFC — DShot HAL Contract
// Each chip family (f4/, f7/, h7/) implements these functions.
// Core logic calls ONLY these; never vendor HAL directly.
//
// The pure-logic frame packing / GCR decode lives in src/dshot_common.hpp.
// This file defines the hardware-side: timer+DMA send, bidir receive.

#pragma once

#include <cstdint>

namespace melty {
namespace hal {

// Initialize DShot timer+DMA for all configured motors.
// Called once from melty_setup(). Target pinmap.h provides pin/timer/DMA mapping.
void dshotInit();

// Send a pre-packed 16-bit DShot frame on one motor channel.
// The frame is already CRC'd by dshot_common — this function only does
// compare-buffer encoding + DMA transfer.
void dshotSend(uint8_t motorIndex, uint16_t frame);

// Commit all pending DShot frames — triggers DMA for all motors.
// Call once per loop after all dshotSend() calls.
void dshotCommit();

// Read bidirectional DShot telemetry (GCR-encoded eRPM from ESC).
// Returns raw 21-bit GCR frame, or 0 if no telemetry received.
// Caller decodes via dshot_common::gcrDecode().
uint32_t dshotReadTelemetryRaw(uint8_t motorIndex);

// Returns true if a new telemetry frame is available since last read.
bool dshotTelemetryReady(uint8_t motorIndex);

// Configure GPIO for bidir turnaround (output → input → output).
// Called by the DMA transfer-complete ISR — must be fast.
void dshotBidirTurnaround(uint8_t motorIndex, bool toInput);

} // namespace hal
} // namespace melty
