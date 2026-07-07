// MeltyFC — Creep/Tank Mode Implementation
// PURE LOGIC — no hardware.

#include "creep.hpp"
#include <cmath>

namespace melty {

bool creepUpdateState(CreepState& state, float currentRpm,
                      bool forceSwitch, const CreepConfig& cfg) {
    state.forcedBySwitch = forceSwitch;

    if (forceSwitch) {
        state.active = true;
        return true;
    }

    if (state.active) {
        // Exit hysteresis: must exceed threshold + hysteresis
        if (currentRpm > static_cast<float>(cfg.thresholdRpm + cfg.hysteresisRpm)) {
            state.active = false;
        }
    } else {
        // Enter: below threshold
        if (currentRpm < static_cast<float>(cfg.thresholdRpm)) {
            state.active = true;
        }
    }

    return state.active;
}

void creepComputeOutput(float stickX, float stickY, float throttleCap,
                        uint8_t numMotors, float* motorOut) {
    // Standard differential drive:
    // left  = stickY + stickX
    // right = stickY - stickX
    float left  = stickY + stickX;
    float right = stickY - stickX;

    // Clamp to [-throttleCap, +throttleCap]
    auto clamp = [throttleCap](float v) -> float {
        if (v > throttleCap) return throttleCap;
        if (v < -throttleCap) return -throttleCap;
        return v;
    };

    if (numMotors >= 2) {
        motorOut[0] = clamp(left);
        motorOut[1] = clamp(right);
    }

    // N=3: third motor passive (0 output — scrub)
    if (numMotors >= 3) {
        motorOut[2] = 0.0f;
    }

    // N=4: fourth motor also passive
    if (numMotors >= 4) {
        motorOut[3] = 0.0f;
    }
}

} // namespace melty
