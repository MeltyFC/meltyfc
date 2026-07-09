// MeltyFC — Slip Detection Implementation

#include "slip.hpp"
#include <algorithm>
#include <cmath>

namespace melty {

float erpmToMechRpm(uint32_t erpm, uint8_t motorPoles) {
    if (motorPoles == 0)
        return 0.0f;
    return static_cast<float>(erpm) / (static_cast<float>(motorPoles) / 2.0f);
}

float wheelImpliedOmega(float mechRpm, const SlipConfig& cfg) {
    // I-15: negated-positive form catches NaN
    if (!(cfg.driveRatio > 0.0f) || !(cfg.wheelMountRadius > 0.0f)) {
        return 0.0f;
    }
    // wheel_rpm = motor_rpm / drive_ratio
    const float wheelRpm = mechRpm / cfg.driveRatio;
    // wheel linear speed = wheel_rpm * π * wheel_dia / 60 (mm/s)
    // bot omega = wheel_speed / wheel_mount_radius
    // Simplify: omega = (wheelRpm / 60) * 2π * (wheelDia/2) / wheelMountRadius
    //         = wheelRpm * π * wheelDia / (60 * wheelMountRadius)
    return wheelRpm * M_PI * cfg.wheelDia / (60.0f * cfg.wheelMountRadius);
}

float computeSlipPct(float omegaAccel, float omegaWheels) {
    if (omegaWheels < 0.1f) {
        return 0.0f; // Avoid division by near-zero
    }
    const float slip = (1.0f - omegaAccel / omegaWheels) * 100.0f;
    return std::clamp(slip, 0.0f, 100.0f);
}

} // namespace melty
