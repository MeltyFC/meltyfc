// MeltyFC — Stick-Referenced Heading Re-Sync Implementation

#include "resync.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace melty {

void resyncInit(ResyncState& state) {
    state.held = false;
    state.sinAccum = 0.0f;
    state.cosAccum = 0.0f;
    state.magAccum = 0.0f;
    state.sampleCount = 0;
    state.holdStartMs = 0;
}

float resyncStickAngle(float stickX, float stickY) {
    return atan2f(stickX, stickY); // 0 = forward (positive Y)
}

float resyncStickMagnitude(float stickX, float stickY) {
    float mag = sqrtf(stickX * stickX + stickY * stickY);
    if (mag > 1.0f)
        mag = 1.0f;
    return mag;
}

float resyncUpdate(ResyncState& state, bool switchHeld, float stickX, float stickY, uint32_t nowMs,
                   const ResyncConfig& cfg) {
    if (switchHeld && !state.held) {
        // Switch just pressed — start accumulating
        state.held = true;
        state.sinAccum = 0.0f;
        state.cosAccum = 0.0f;
        state.magAccum = 0.0f;
        state.sampleCount = 0;
        state.holdStartMs = nowMs;
    }

    if (switchHeld && state.held) {
        float angle = resyncStickAngle(stickX, stickY);
        float mag = resyncStickMagnitude(stickX, stickY);

        // Circular mean: accumulate sin/cos components to handle wrapping
        // correctly. Arithmetic mean of angles breaks at ±180° (the exact
        // post-crash case this feature exists for).
        state.sinAccum += sinf(angle);
        state.cosAccum += cosf(angle);
        state.magAccum += mag;
        state.sampleCount++;
        return 0.0f; // Still held — no offset yet
    }

    if (!switchHeld && state.held) {
        // Switch released — compute offset via circular mean
        state.held = false;

        if (state.sampleCount == 0)
            return 0.0f;

        float avgMag = state.magAccum / static_cast<float>(state.sampleCount);

        // Check minimum deflection
        if (avgMag < cfg.minDeflection) {
            return 0.0f; // Cancel — stick wasn't deflected enough
        }

        // Circular mean: atan2(sum_sin, sum_cos)
        float avgAngle = atan2f(state.sinAccum, state.cosAccum);

        return avgAngle;
    }

    return 0.0f;
}

} // namespace melty
