// MeltyFC — Heading Engine Implementation
// PURE LOGIC — no hardware includes.

#include "heading.hpp"
#include <algorithm>
#include <cmath>

namespace melty {

// DI-17: NaN guard — returns 0.0 if any input is non-finite
static inline float safeFloat(float v, float fallback = 0.0f) {
    return std::isfinite(v) ? v : fallback;
}

float computeOmegaDifferential(float aOuter, float aInner, float drEff) {
    // DI-17: Reject NaN inputs
    aOuter = safeFloat(aOuter);
    aInner = safeFloat(aInner);
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

float integratePhase(float phase, float omega, float dt, float direction) {
    // C2: direction = +1.0 (CW) or -1.0 (CCW).
    // When inverted, physical rotation reverses — direction flips so phase
    // tracks the real world. omega is always positive (from sqrt), direction
    // carries the sign.
    phase += direction * omega * dt;
    // Wrap to [0, 2π)
    phase = fmodf(phase, 2.0f * static_cast<float>(M_PI));
    if (phase < 0.0f)
        phase += 2.0f * static_cast<float>(M_PI);
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

    // C1: Motor POSITION is where the motor sits on the bot.
    // Motor THRUST direction is perpendicular to the radial line —
    // 90° ahead in the spin direction. The translation window must
    // compare the THRUST angle to the commanded direction, not the
    // position angle. This +π/2 offset is the key geometric correction.
    const float thrustAngle = motorAngle + static_cast<float>(M_PI) / 2.0f;

    // Compute angular distance from thrust direction to translation direction
    float angleDiff = thrustAngle - transAngle;
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
    constexpr float DEADBAND = 0.02f;
    const float absStick = fabsf(stickVal);
    if (absStick < DEADBAND)
        return 0.0f;

    // D9: Remap stick from [deadband, 1.0] → [0, 1.0] to eliminate
    // the discontinuity at the deadband edge. Without this, rate jumps
    // from 0 to rateFine instantly.
    const float remapped = (absStick - DEADBAND) / (1.0f - DEADBAND);

    // Expo curve: mix linear and cubic
    const float linear = remapped;
    const float cubic = remapped * remapped * remapped;
    const float curved = linear * (1.0f - cfg.expo) + cubic * cfg.expo;

    // Map to rate range: 0→rateFine, 1→rateMax (smooth from deadband edge)
    const float rate = cfg.rateFine + curved * (cfg.rateMax - cfg.rateFine);
    return copysignf(rate, stickVal);
}

float computeRpmHold(float currentRpm, float targetRpm, float baseThrottle,
                     const RpmHoldConfig& cfg, RpmHoldState* state) {
    if (!cfg.enabled || targetRpm <= 0.0f) {
        if (state)
            state->wasEnabled = false;
        return baseThrottle;
    }

    // D5: On first engagement, capture the current throttle as feedforward
    // to avoid a step from baseThrottle to feedforward+correction.
    float ff = cfg.feedforward;
    if (state) {
        if (!state->wasEnabled) {
            state->capturedThrottle = baseThrottle;
            state->wasEnabled = true;
        }
        ff = state->capturedThrottle;
    }

    const float error = targetRpm - currentRpm;
    const float correction = cfg.kp * error;
    const float output = ff + correction;

    return std::clamp(output, 0.0f, 1.0f);
}

float applyInversion(float transAngle, bool inverted) {
    if (inverted) {
        // Mirror the translation angle across the spin axis
        return -transAngle;
    }
    return transAngle;
}

bool detectHit(float aOuterRaw, float expectedG, float thresholdG, float omegaRadS,
               float minOmegaForHitDetection, float commandedDOmega,
               float maxDOmegaForHitDetection) {
    // A6: Suppress hit detection during commanded spin-up/ramp.
    // During ramp, true ω leads slew-limited estimate → residual exceeds
    // threshold → false-positive blanking at every launch.
    if (omegaRadS < minOmegaForHitDetection)
        return false;
    if (fabsf(commandedDOmega) > maxDOmegaForHitDetection)
        return false;
    return fabsf(aOuterRaw - expectedG) > thresholdG;
}

} // namespace melty
