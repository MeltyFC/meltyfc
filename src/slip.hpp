// MeltyFC — Slip Detection
// PURE LOGIC — compares accel-derived omega vs wheel-implied omega from eRPM.
// See spec §9.

#pragma once

#include <cstdint>

namespace melty {

struct SlipConfig {
    float driveRatio;       // Bell:wheel gear ratio
    float wheelDia;         // mm
    float wheelMountRadius; // mm (distance from spin axis to wheel contact)
    uint8_t motorPoles;     // For eRPM → mechanical RPM conversion
    float warnPct;          // Slip % threshold for warning
    uint16_t warnMs;        // Sustained duration before warning fires
};

struct SlipResult {
    float slipPct;          // 0–100%, per motor or aggregate
    bool  warning;          // Sustained slip above threshold
    float omegaWheels;      // Wheel-implied bot omega (rad/s)
};

// Convert eRPM to mechanical RPM: mech_rpm = erpm / (poles / 2)
float erpmToMechRpm(uint32_t erpm, uint8_t motorPoles);

// Compute wheel-implied bot omega from motor mechanical RPM
// omega_wheels = motor_rpm / drive_ratio * (wheel_dia/2) / wheel_mount_radius
// All linear dimensions in mm (they cancel)
float wheelImpliedOmega(float mechRpm, const SlipConfig& cfg);

// Compute slip percentage: slip = 1 - omega_accel / omega_wheels
// Returns 0–100%. Clamped: >100% means negative (wheels faster than bot),
// <0% shouldn't happen (bot faster than wheels — would mean external force)
float computeSlipPct(float omegaAccel, float omegaWheels);

} // namespace melty
