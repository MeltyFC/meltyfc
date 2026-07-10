// MeltyFC — Safety & Arming State Machine
// See spec §10. SPARC-derived requirements.
// Round 4 additions: LQ failsafe, arm debounce, valid-frame gating,
// hot-plug disarm, choke point enforcement.

#pragma once

#include <cstdint>
#include <cmath>  // B1/I-37: isfinite for choke NaN guard

namespace melty {

enum class ArmState : uint8_t {
    BOOT,     // Initial state — waiting for preconditions
    DISARMED, // Safe — arm switch seen LOW, ready to arm
    ARMED,    // Live — motors can spin
    FAILSAFE, // RC link lost — motors OFF, requires re-arm
    ERROR,    // Sensor fault or critical error
};

struct ArmPreconditions {
    bool armSwitchLow;           // Arm switch was seen LOW since boot (from VALID frame)
    bool spinStickZero;          // Spin throttle at zero (from VALID frame)
    bool rcLinkHealthy;          // CRSF frames arriving
    bool sensorsHealthy;         // Both H3LIS + ICM responding
    bool armSwitchCurrentlyHigh; // Arm switch is HIGH right now (from VALID frame)
    bool frameValid;             // Current frame passed CRC + is fresh (< 100ms)
    uint8_t linkQuality;         // CRSF uplink LQ (0-100, 0 = link lost)
    bool batteryPresent;         // VBAT > threshold (hot-plug detection)
    // Finding 3/5: LVC fail-closed fields
    bool vbatValid;   // ADC reading is sane (not disconnected/shorted)
    bool lvcCritical; // LVC level is CRITICAL — force disarm
    bool configValid; // Config passed cross-param validation
};

struct SafetyConfig {
    uint16_t failsafeMs;       // Max time without RC frame before failsafe
    uint16_t watchdogMs;       // IWDG timeout
    uint8_t armDebounceFrames; // Consecutive high frames required to arm (default 5)
};

struct SafetyState {
    uint8_t armHighCount;   // Consecutive valid frames with arm switch high
    bool batteryWasPresent; // Battery presence on previous cycle
};

// Check if all arming preconditions are met
// Now requires valid-frame gating + debounce
bool canArm(const ArmPreconditions& pre, const SafetyState& safety, uint8_t requiredFrames);

// State transitions — returns new state given current state and conditions
// 2a: LQ-based failsafe (linkQuality == 0 triggers regardless of RC frames)
// 2b: Arm debounce (N consecutive frames)
// 2c: Valid-frame gating (preconditions only from CRC-valid, fresh frames)
// 2g: Hot-plug disarm (battery appears while armed → force disarm)
ArmState updateArmState(ArmState current, const ArmPreconditions& pre, SafetyState& safety,
                        uint32_t msSinceLastRcFrame, const SafetyConfig& cfg);

// Returns true if motors are allowed to output nonzero throttle
inline bool motorsAllowed(ArmState state) {
    return state == ArmState::ARMED;
}

// Apply the choke point: output = 0 unless ARMED
// This is THE ONLY function that should write motor values.
// verify.sh grep-gates that only motors_dshot.cpp calls this.
inline float chokeMotorOutput(float mixerOutput, ArmState state) {
    // B1/I-37: NaN/Inf → 0 (last line of defense)
    if (!std::isfinite(mixerOutput))
        return 0.0f;
    return motorsAllowed(state) ? mixerOutput : 0.0f;
}

// Map normalized throttle (0.0-1.0) to DShot value: {0} ∪ [48, 2047]
// Values 1-47 are ESC commands — NEVER emitted as throttle.
inline uint16_t throttleToDshot(float normalized, ArmState state) {
    // B1/I-37: NaN/Inf → 0 (last line of defense)
    if (!std::isfinite(normalized))
        return 0;
    if (!motorsAllowed(state))
        return 0;
    if (normalized <= 0.0f)
        return 0;
    // Map 0.0-1.0 → 48-2047
    uint16_t val = static_cast<uint16_t>(48.0f + normalized * 1999.0f);
    if (val < 48)
        val = 48;
    if (val > 2047)
        val = 2047;
    return val;
}

} // namespace melty
