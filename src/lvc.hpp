// MeltyFC — Low Voltage Cutoff
// PURE LOGIC — per-cell voltage calc, auto-detect cell count, warn/crit thresholds.
// See spec §9C.

#pragma once

#include <cstdint>

namespace melty {

// ============================================================================
// LVC state
// ============================================================================
enum class LvcLevel : uint8_t {
    OK,
    WARN,     // Below warn threshold — TX warning, LED overlay
    CRITICAL, // Below crit threshold — auto spin-down
};

struct LvcConfig {
    float warnVolts;   // Per-cell warn threshold (V)
    float critVolts;   // Per-cell crit threshold (V)
    uint8_t cellCount; // 0 = auto-detect
};

struct LvcState {
    uint8_t detectedCells; // Auto-detected cell count
    float packVoltage;     // Total pack voltage (V)
    float cellVoltage;     // Per-cell voltage (V)
    LvcLevel level;
    bool spinDownActive; // Auto spin-down triggered
};

// ============================================================================
// LVC functions
// ============================================================================

// Auto-detect cell count from pack voltage.
// Uses nominal ranges: 1S=3.0-4.35, 2S=6.0-8.7, 3S=9.0-13.05, 4S=12.0-17.4
// Returns 0 if voltage too low or ambiguous.
uint8_t lvcAutoDetectCells(float packVoltage);

// Update LVC state.
// packVoltage: measured pack voltage
// cfg: LVC configuration
// Returns the LVC level.
LvcLevel lvcUpdate(LvcState& state, float packVoltage, const LvcConfig& cfg);

// Get the capped throttle for spin-down mode.
// Returns a throttle multiplier (0.0 to 1.0).
// When critical: ramps down over ~2 seconds.
float lvcSpinDownThrottle(const LvcState& state, float currentThrottle, uint32_t critDurationMs);

} // namespace melty
