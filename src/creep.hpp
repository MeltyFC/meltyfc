// MeltyFC — Creep/Tank Mode
// PURE LOGIC — differential drive math for below-spin-threshold control.
// N=2 wheels = standard diff-drive (full quality)
// N=3 = two-wheel drive w/ third-wheel scrub (degraded, documented)
// Entry: hybrid with hysteresis + CH_CREEP_FORCE switch.
// See spec §9C.

#pragma once

#include <cstdint>

namespace melty {

// ============================================================================
// Creep mode state
// ============================================================================
struct CreepState {
    bool active;         // Currently in creep mode
    bool forcedBySwitch; // Forced by CH_CREEP_FORCE
};

struct CreepConfig {
    uint16_t thresholdRpm;  // Enter creep below this RPM
    uint16_t hysteresisRpm; // Exit = threshold + hysteresis
    uint8_t numMotors;      // 2 or 3
};

// ============================================================================
// Creep mode transition — hybrid with hysteresis
// ============================================================================

// Update creep mode state.
// currentRpm: measured RPM (0 if stopped)
// forceSwitch: CH_CREEP_FORCE active
// Returns true if creep mode should be active.
[[nodiscard]] bool creepUpdateState(CreepState& state, float currentRpm, bool forceSwitch,
                      const CreepConfig& cfg);

// ============================================================================
// Differential drive output
// ============================================================================

// Compute motor outputs for creep/tank mode.
// stickX: left/right (-1..+1) — turn
// stickY: forward/back (-1..+1) — drive
// throttleCap: max output (0..1)
// motorOut[]: output array (sized for numMotors)
// N=2: standard diff drive (motors at 0°/180°)
// N=3: motors 0+1 drive, motor 2 passive (scrub)
// A7: inverted param — when bot is upside down, stickX must negate
// so that "left" on the TX still means "left" in the world.
void creepComputeOutput(float stickX, float stickY, float throttleCap, uint8_t numMotors,
                        float* motorOut, bool inverted = false);

} // namespace melty
