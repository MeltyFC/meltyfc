// MeltyFC — VBAT HAL Contract
// Each chip family implements ADC reading for battery voltage.
// Core logic calls ONLY these; never vendor ADC HAL directly.
//
// R7-1: The voltage class has a brain (lvc.cpp) and no senses.
// This contract provides the senses.
//
// Per-family ADC resolution:
//   F4/F7: 12-bit (0-4095)
//   H7: 16-bit (0-65535)
// The HAL normalizes to millivolts — core never sees raw ADC counts.
//
// Per-target voltage divider ratio:
//   VBAT_DIVIDER_RATIO defined in target pinmap.h (from BF vbat_scale)
//   vbatReadMv() applies the divider internally.
//
// Disarm-frame prefill equivalent for VBAT:
//   vbatInit() must leave vbatValid=false until first valid reading.
//   Safety precondition (batteryPresent + vbatValid) blocks arming
//   until the ADC has produced at least one in-range sample.

#pragma once

#include <cstdint>

namespace melty {
namespace hal {

// Initialize ADC for VBAT reading on the pin defined in target pinmap.
// Sets vbatValid=false until first valid reading.
void vbatInit();

// Read battery voltage in millivolts.
// Applies the per-target VBAT_DIVIDER_RATIO internally.
// Returns 0 if ADC not initialized or reading invalid.
uint32_t vbatReadMv();

// Returns true if the ADC is initialized and producing valid readings.
// Used by safety.cpp arming precondition (vbatValid).
bool vbatValid();

// Read raw ADC count (for diagnostics / CLI display).
// NOT for voltage calculation — use vbatReadMv() for that.
uint16_t vbatReadRaw();

// Per-family ADC full-scale value
// F4/F7: 4095 (12-bit). H7: 65535 (16-bit).
// Defined per family, NOT hardcoded in core.
uint16_t vbatAdcFullScale();

} // namespace hal
} // namespace melty
