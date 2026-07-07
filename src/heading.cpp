// MeltyFC — Heading Engine Implementation
// PURE LOGIC — no hardware includes.

#include "heading.hpp"
#include <algorithm>
#include <cmath>

namespace melty {

float computeOmegaDifferential(float aOuter, float aInner, float drEff) {
    // ω = sqrt(max(0, (a_outer - a_inner)) / dr_eff)
    // Acceleration in m/s² (input in g, convert: a_g * 9.81)
    // dr_eff in meters
    const float diff = aOuter - aInner;
    if (diff <= 0.0f || drEff <= 0.0f) {
        return 0.0f;
    }
    // a is in g, convert to m/s²: diff * 9.80665
    // ω² = a_centripetal / r = (diff_g * 9.80665) / dr_eff_m
    return sqrtf((diff * 9.80665f) / drEff);
}

float slewLimitOmega(float newOmega, float prevOmega, float maxSlew, float dt) {
    const float maxDelta = maxSlew * dt;
    const float delta = newOmega - prevOmega;
    if (fabsf(delta) > maxDelta) {
        return prevOmega + copysignf(maxDelta, delta);
    }
    return newOmega;
}

float integratePhase(float phase, float omega, float dt) {
    phase += omega * dt;
    // Wrap to [0, 2π)
    phase = fmodf(phase, 2.0f * M_PI);
    if (phase < 0.0f)
        phase += 2.0f * M_PI;
    return phase;
}

float computeTranslationAngle(float stickX, float stickY) {
    return atan2f(stickX, stickY); // 0 = forward (+Y), CW positive
}

float computeTranslationMagnitude(float stickX, float stickY, float deadband) {
    const float mag = sqrtf(stickX * stickX + stickY * stickY);
    if (mag < deadband)
        return 0.0f;
    // Scale so that deadband edge = 0, full deflection = 1
    return std::min(1.0f, (mag - deadband) / (1.0f - deadband));
}

float computeMotorOutput(float motorAngle, float transAngle, float transMag, float spinThrottle,
                         float windowHalf, float throttleCap) {
    if (transMag < 0.001f) {
        // No translation — spin only
        return std::min(spinThrottle, throttleCap);
    }

    // Compute angular distance from motor to translation direction
    float angleDiff = motorAngle - transAngle;
    // Normalize to [-π, π)
    angleDiff = fmodf(angleDiff + M_PI, 2.0f * M_PI);
    if (angleDiff < 0.0f)
        angleDiff += 2.0f * M_PI;
    angleDiff -= M_PI;

    const float absDiff = fabsf(angleDiff);

    float output = spinThrottle;

    if (absDiff < windowHalf) {
        // Inside boost window — add translation component
        // Scale boost by how centered we are in the window
        const float windowFraction = 1.0f - (absDiff / windowHalf);
        output += transMag * (throttleCap - spinThrottle) * windowFraction;
    } else if (absDiff > (M_PI - windowHalf)) {
        // Inside opposing window — reduce (OpenMelt2-style modulation)
        const float oppFraction = 1.0f - ((M_PI - absDiff) / windowHalf);
        output -= transMag * spinThrottle * 0.5f * oppFraction;
    }

    return std::clamp(output, 0.0f, throttleCap);
}

float computeTrimRate(float stickVal, const TrimConfig& cfg) {
    const float absStick = fabsf(stickVal);
    if (absStick < 0.02f)
        return 0.0f; // Tiny deadband

    // Expo curve: mix linear and cubic
    const float linear = absStick;
    const float cubic = absStick * absStick * absStick;
    const float curved = linear * (1.0f - cfg.expo) + cubic * cfg.expo;

    // Map to rate range
    const float rate = cfg.rateFine + curved * (cfg.rateMax - cfg.rateFine);
    return copysignf(rate, stickVal);
}

float computeRpmHold(float currentRpm, float targetRpm, float baseThrottle,
                     const RpmHoldConfig& cfg) {
    if (!cfg.enabled || targetRpm <= 0.0f) {
        return baseThrottle;
    }

    const float error = targetRpm - currentRpm;
    const float correction = cfg.kp * error;
    const float output = cfg.feedforward + correction;

    return std::clamp(output, 0.0f, 1.0f);
}

float applyInversion(float transAngle, bool inverted) {
    if (inverted) {
        // Mirror the translation angle across the spin axis
        return -transAngle;
    }
    return transAngle;
}

bool detectHit(float aOuterRaw, float expectedG, float thresholdG) {
    return fabsf(aOuterRaw - expectedG) > thresholdG;
}

} // namespace melty
