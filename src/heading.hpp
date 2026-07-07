// MeltyFC — Heading Engine
// PURE LOGIC — no hardware includes. Takes sensor readings via parameters.
// Implements: differential omega, phase integration, heading trim, RPM hold.
// See spec §5, §6, §9C (RPM hold).

#pragma once

#include <cmath>
#include <cstdint>

namespace melty {

struct HeadingState {
    float omega;         // Current spin rate (rad/s)
    float phase;         // Current bot angle vs world (0..2π)
    float headingOffset; // Trim offset (rad)
    bool sensorRailed;   // Outer accel near full scale
    bool hitDetected;    // Impact detected this cycle
    bool gyroBlanking;   // Gyro readings suppressed (hit window)
    bool lowSpeedMode;   // Using ICM instead of H3LIS
};

struct HeadingConfig {
    float drEff;             // Effective radius difference (m)
    float omegaSlewMax;      // Max omega change rate (rad/s²)
    float hitThreshG;        // Hit detection threshold (g)
    float gyroBlankMs;       // Gyro blanking duration (ms)
    float lowspeedThreshRpm; // Switch to low-speed source below this
    float railedThreshold;   // Fraction of full scale (e.g., 0.95)
    float fullScaleG;        // H3LIS full scale (400g)
};

struct RpmHoldConfig {
    bool enabled;
    float targetRpm;
    float kp;          // P gain
    float feedforward; // Base throttle fraction
};

struct TrimConfig {
    float rateFine; // deg/s at min deflection
    float rateMax;  // deg/s at full deflection
    float expo;     // 0.0=linear, 1.0=max expo
};

// ============================================================================
// Pure functions — testable without hardware
// ============================================================================

// Differential omega from two accelerometer readings
// Returns omega in rad/s, clamped to >= 0
float computeOmegaDifferential(float aOuter, float aInner, float drEff);

// Apply slew-rate limit to omega
float slewLimitOmega(float newOmega, float prevOmega, float maxSlew, float dt);

// Integrate phase — wraps to [0, 2π)
// direction: +1.0 for CW, -1.0 for CCW (from SPIN_DIRECTION XOR inverted)
// C2 fix: when inverted, physical rotation reverses — phase must track.
float integratePhase(float phase, float omega, float dt, float direction = 1.0f);

// Compute translation direction from stick X/Y (returns radians, 0 = forward)
float computeTranslationAngle(float stickX, float stickY);

// Compute translation magnitude from stick X/Y (0.0–1.0)
float computeTranslationMagnitude(float stickX, float stickY, float deadband);

// Per-motor throttle with translation modulation
// motorAngle: world angle of this motor right now (phase + offset + mounting angle)
// transAngle: commanded translation direction (rad)
// transMag: translation strength (0–1)
// spinThrottle: base spin throttle (0–1)
// windowHalf: translation window half-width (rad)
// throttleCap: absolute max output (0–1)
float computeMotorOutput(float motorAngle, float transAngle, float transMag, float spinThrottle,
                         float windowHalf, float throttleCap);

// Heading trim rate from stick deflection (with expo curve)
// stickVal: -1.0 to 1.0
float computeTrimRate(float stickVal, const TrimConfig& cfg);

// RPM hold governor — returns adjusted throttle
// P + feedforward, no integral (no-brake plant, sag is slow)
float computeRpmHold(float currentRpm, float targetRpm, float baseThrottle,
                     const RpmHoldConfig& cfg);

// Apply inversion — negate translation angle
float applyInversion(float transAngle, bool inverted);

// Hit detection from accel readings
// A6: Gated on ω > minimum AND |commanded dω| < ramp threshold
// to prevent false-fires during every spin-up.
bool detectHit(float aOuterRaw, float expectedG, float thresholdG, float omegaRadS = 999.0f,
               float minOmegaForHitDetection = 0.0f, float commandedDOmega = 0.0f,
               float maxDOmegaForHitDetection = 999.0f);

// Omega from RPM and vice versa
inline float rpmToOmega(float rpm) {
    return rpm * (2.0f * M_PI / 60.0f);
}
inline float omegaToRpm(float omega) {
    return omega * (60.0f / (2.0f * M_PI));
}

} // namespace melty
