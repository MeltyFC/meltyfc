// MeltyFC — Safety & Arming Implementation

#include "safety.hpp"

namespace melty {

bool canArm(const ArmPreconditions& pre) {
    return pre.armSwitchLow               // Switch was seen LOW after boot
           && pre.spinStickZero           // Spin throttle is zero
           && pre.rcLinkHealthy           // CRSF link up
           && pre.sensorsHealthy          // All sensors responding
           && pre.armSwitchCurrentlyHigh; // Switch is NOW requesting arm
}

ArmState updateArmState(ArmState current, const ArmPreconditions& pre, uint32_t msSinceLastRcFrame,
                        uint16_t failsafeMs) {
    // Failsafe overrides everything except ERROR
    if (current != ArmState::ERROR && msSinceLastRcFrame > failsafeMs) {
        return ArmState::FAILSAFE;
    }

    // Sensor fault → ERROR from any state
    if (!pre.sensorsHealthy) {
        return ArmState::ERROR;
    }

    switch (current) {
    case ArmState::BOOT:
        // Transition to DISARMED once arm switch is seen LOW
        if (pre.armSwitchLow && !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED;
        }
        return ArmState::BOOT;

    case ArmState::DISARMED:
        if (canArm(pre)) {
            return ArmState::ARMED;
        }
        return ArmState::DISARMED;

    case ArmState::ARMED:
        // Disarm if switch goes LOW
        if (!pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED;
        }
        return ArmState::ARMED;

    case ArmState::FAILSAFE:
        // Recovery requires: link restored + full re-arm gesture
        if (pre.rcLinkHealthy && pre.armSwitchLow && !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED; // Must re-arm, not auto-resume
        }
        return ArmState::FAILSAFE;

    case ArmState::ERROR:
        // Recovery: sensors come back + arm switch low
        if (pre.sensorsHealthy && pre.armSwitchLow && !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED;
        }
        return ArmState::ERROR;
    }

    return ArmState::ERROR; // Should never reach
}

} // namespace melty
