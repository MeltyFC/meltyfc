// MeltyFC — Safety & Arming Implementation
// Round 4 safety fault tree mitigations.

#include "safety.hpp"

namespace melty {

bool canArm(const ArmPreconditions& pre, const SafetyState& safety, uint8_t requiredFrames) {
    // 2c: ALL preconditions must come from valid, fresh frames
    if (!pre.frameValid)
        return false;

    return pre.armSwitchLow              // Switch was seen LOW after boot (from valid frame)
           && pre.spinStickZero          // Spin throttle is zero (from valid frame)
           && pre.rcLinkHealthy          // CRSF link up
           && pre.sensorsHealthy         // All sensors responding
           && pre.armSwitchCurrentlyHigh // Switch is NOW requesting arm (from valid frame)
           && pre.linkQuality > 0        // 2a: LQ must be nonzero
           && pre.batteryPresent         // Battery must be present
           && pre.vbatValid              // Finding 3: VBAT ADC must be sane
           && !pre.lvcCritical           // Finding 5: cannot arm while LVC critical
           && pre.configValid            // Finding 9: config must pass validation
           && safety.armHighCount >= requiredFrames; // 2b: debounce
}

ArmState updateArmState(ArmState current, const ArmPreconditions& pre, SafetyState& safety,
                        uint32_t msSinceLastRcFrame, const SafetyConfig& cfg) {

    // --- 2a: LQ-based failsafe (defeats all RX failsafe modes) ---
    // LQ == 0 means no uplink packets received regardless of whether
    // the RX is still emitting frames (Last Position / Set Position modes).
    if (current != ArmState::ERROR && current != ArmState::BOOT) {
        if (pre.linkQuality == 0 || msSinceLastRcFrame > cfg.failsafeMs) {
            safety.armHighCount = 0; // Reset debounce
            return ArmState::FAILSAFE;
        }
    }

    // --- Finding 5: Hard safety failures force disarm from any running state ---
    if (current != ArmState::BOOT && current != ArmState::ERROR) {
        if (pre.lvcCritical || !pre.vbatValid || !pre.configValid) {
            safety.armHighCount = 0;
            return ArmState::ERROR;
        }
    }

    // --- 2g: Hot-plug disarm ---
    // Battery appears while armed → force disarm (workshop injury prevention)
    if (current == ArmState::ARMED) {
        if (!safety.batteryWasPresent && pre.batteryPresent) {
            safety.armHighCount = 0;
            safety.batteryWasPresent = pre.batteryPresent;
            return ArmState::DISARMED;
        }
    }
    safety.batteryWasPresent = pre.batteryPresent;

    // --- Sensor fault → ERROR from any running state ---
    if (current != ArmState::BOOT && !pre.sensorsHealthy) {
        safety.armHighCount = 0;
        return ArmState::ERROR;
    }

    // --- 2b: Arm debounce counter ---
    // Only count from valid frames (2c)
    if (pre.frameValid && pre.armSwitchCurrentlyHigh) {
        if (safety.armHighCount < 255)
            safety.armHighCount++;
    } else {
        safety.armHighCount = 0; // Any non-high or invalid frame resets
    }

    switch (current) {
    case ArmState::BOOT:
        // Transition to DISARMED once arm switch is seen LOW in a valid frame
        if (pre.frameValid && pre.armSwitchLow && !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED;
        }
        return ArmState::BOOT;

    case ArmState::DISARMED:
        if (canArm(pre, safety, cfg.armDebounceFrames)) {
            return ArmState::ARMED;
        }
        return ArmState::DISARMED;

    case ArmState::ARMED:
        // Disarm INSTANTLY on switch low (asymmetric: hard to arm, instant to disarm)
        if (!pre.armSwitchCurrentlyHigh) {
            safety.armHighCount = 0;
            return ArmState::DISARMED;
        }
        return ArmState::ARMED;

    case ArmState::FAILSAFE:
        // Recovery requires: LQ restored + link restored + full re-arm gesture
        if (pre.rcLinkHealthy && pre.linkQuality > 0 && pre.frameValid && pre.armSwitchLow &&
            !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED; // Must re-arm, not auto-resume
        }
        return ArmState::FAILSAFE;

    case ArmState::ERROR:
        // Recovery: sensors come back + arm switch low (from valid frame)
        if (pre.sensorsHealthy && pre.frameValid && pre.armSwitchLow &&
            !pre.armSwitchCurrentlyHigh) {
            return ArmState::DISARMED;
        }
        return ArmState::ERROR;
    }

    return ArmState::ERROR; // Should never reach
}

} // namespace melty
