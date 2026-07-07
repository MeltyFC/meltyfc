// MeltyFC — Safety & Arming State Machine
// See spec §10. SPARC-derived requirements.

#pragma once

#include <cstdint>

namespace melty {

enum class ArmState : uint8_t {
    BOOT,           // Initial state — waiting for preconditions
    DISARMED,       // Safe — arm switch seen LOW, ready to arm
    ARMED,          // Live — motors can spin
    FAILSAFE,       // RC link lost — motors OFF, requires re-arm
    ERROR,          // Sensor fault or critical error
};

struct ArmPreconditions {
    bool armSwitchLow;          // Arm switch was seen LOW since boot
    bool spinStickZero;         // Spin throttle at zero
    bool rcLinkHealthy;         // CRSF frames arriving
    bool sensorsHealthy;        // Both H3LIS + ICM responding
    bool armSwitchCurrentlyHigh;// Arm switch is HIGH right now (requesting arm)
};

struct SafetyConfig {
    uint16_t failsafeMs;        // Max time without RC frame before failsafe
    uint16_t watchdogMs;        // IWDG timeout
};

// Check if all arming preconditions are met
bool canArm(const ArmPreconditions& pre);

// State transitions — returns new state given current state and conditions
ArmState updateArmState(ArmState current, const ArmPreconditions& pre,
                        uint32_t msSinceLastRcFrame, uint16_t failsafeMs);

// Returns true if motors are allowed to output nonzero throttle
inline bool motorsAllowed(ArmState state) {
    return state == ArmState::ARMED;
}

} // namespace melty
