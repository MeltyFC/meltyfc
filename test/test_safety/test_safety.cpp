// MeltyFC — Safety & Arming Unit Tests
// Covers Round 4 safety fault tree: LQ failsafe, arm debounce,
// valid-frame gating, hot-plug disarm, choke point, throttle mapping.

#include "safety.hpp"
#include <unity.h>

using namespace melty;

static constexpr uint8_t DEFAULT_DEBOUNCE = 5;

static ArmPreconditions allGood() {
    return {
        true,  // armSwitchLow
        true,  // spinStickZero
        true,  // rcLinkHealthy
        true,  // sensorsHealthy
        true,  // armSwitchCurrentlyHigh
        true,  // frameValid
        100,   // linkQuality
        true,  // batteryPresent
        true,  // vbatValid (Finding 3)
        false, // lvcCritical (Finding 5)
        true,  // configValid (Finding 9)
    };
}

static SafetyConfig defaultCfg() {
    return {500, 200, DEFAULT_DEBOUNCE};
}

static SafetyState freshState() {
    return {DEFAULT_DEBOUNCE, true}; // Pre-debounced for basic tests, battery was present
}

// ============================================================================
// Arming preconditions
// ============================================================================

void test_can_arm_all_good() {
    auto safety = freshState();
    TEST_ASSERT_TRUE(canArm(allGood(), safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_switch_never_low() {
    auto pre = allGood();
    pre.armSwitchLow = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_spin_stick_nonzero() {
    auto pre = allGood();
    pre.spinStickZero = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_no_rc_link() {
    auto pre = allGood();
    pre.rcLinkHealthy = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_sensors_unhealthy() {
    auto pre = allGood();
    pre.sensorsHealthy = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_switch_not_high() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

// 2c: Invalid frame → cannot arm
void test_cannot_arm_invalid_frame() {
    auto pre = allGood();
    pre.frameValid = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

// 2a: LQ=0 → cannot arm
void test_cannot_arm_lq_zero() {
    auto pre = allGood();
    pre.linkQuality = 0;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

// 2b: Insufficient debounce → cannot arm
void test_cannot_arm_insufficient_debounce() {
    auto pre = allGood();
    SafetyState safety = {3, true}; // Only 3 frames, need 5
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

// 2g: No battery → cannot arm
void test_cannot_arm_no_battery() {
    auto pre = allGood();
    pre.batteryPresent = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

// 2c: Default-initialized preconditions → all false (no phantom latch)
void test_preconditions_default_init_all_false() {
    ArmPreconditions pre = {}; // Zero-init
    SafetyState safety = {};
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
    // Specifically: frameValid=false blocks everything
    TEST_ASSERT_FALSE(pre.frameValid);
}

// ============================================================================
// State machine transitions
// ============================================================================

void test_boot_to_disarmed() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false; // Switch LOW
    SafetyState safety = {};
    auto state = updateArmState(ArmState::BOOT, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_boot_stays_boot_switch_high() {
    auto pre = allGood();
    pre.armSwitchLow = false; // Never seen LOW
    SafetyState safety = {};
    auto state = updateArmState(ArmState::BOOT, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::BOOT, state);
}

void test_disarmed_to_armed() {
    auto pre = allGood();
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::DISARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::ARMED, state);
}

void test_armed_to_disarmed_switch_low() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false; // Switch LOW
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_failsafe_on_link_loss() {
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, allGood(), safety, 600, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::FAILSAFE, state);
}

// 2a: LQ=0 triggers failsafe even with valid frames arriving
void test_failsafe_on_lq_zero() {
    auto pre = allGood();
    pre.linkQuality = 0; // LQ dead but frames still coming (RX failsafe mode)
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::FAILSAFE, state);
}

void test_failsafe_requires_rearm() {
    auto pre = allGood(); // Link restored but switch still HIGH
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::FAILSAFE, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::FAILSAFE, state); // Must see switch LOW first
}

void test_failsafe_recovery() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false; // Switch LOW
    SafetyState safety = {};
    auto state = updateArmState(ArmState::FAILSAFE, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_sensor_fault_to_error() {
    auto pre = allGood();
    pre.sensorsHealthy = false;
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::ERROR, state);
}

void test_error_recovery() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false; // Switch LOW
    SafetyState safety = {};
    auto state = updateArmState(ArmState::ERROR, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_never_spin_on_boot() {
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::BOOT));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::DISARMED));
    TEST_ASSERT_TRUE(motorsAllowed(ArmState::ARMED));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::FAILSAFE));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::ERROR));
}

// ============================================================================
// Round 4: Arm debounce (T2)
// ============================================================================

void test_arm_debounce_4_high_1_low_4_high_never_arms() {
    // T2: 4 high frames, 1 low, 4 high → never reaches 5 consecutive → never arms
    SafetyState safety = {0, true};
    SafetyConfig cfg = defaultCfg();
    auto pre = allGood();

    // 4 high frames
    for (int i = 0; i < 4; i++) {
        updateArmState(ArmState::DISARMED, pre, safety, 0, cfg);
    }
    TEST_ASSERT_EQUAL_UINT8(4, safety.armHighCount); // Not yet 5

    // 1 low frame — resets counter
    pre.armSwitchCurrentlyHigh = false;
    updateArmState(ArmState::DISARMED, pre, safety, 0, cfg);
    TEST_ASSERT_EQUAL_UINT8(0, safety.armHighCount);

    // 4 more high frames
    pre.armSwitchCurrentlyHigh = true;
    ArmState state = ArmState::DISARMED;
    for (int i = 0; i < 4; i++) {
        state = updateArmState(ArmState::DISARMED, pre, safety, 0, cfg);
    }
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state); // Still not armed
}

void test_arm_debounce_5_consecutive_arms() {
    SafetyState safety = {0, true};
    SafetyConfig cfg = defaultCfg();
    auto pre = allGood();

    ArmState state = ArmState::DISARMED;
    for (int i = 0; i < 5; i++) {
        state = updateArmState(ArmState::DISARMED, pre, safety, 0, cfg);
    }
    // After 5 consecutive high frames, should arm
    TEST_ASSERT_EQUAL(ArmState::ARMED, state);
}

// ============================================================================
// Finding 3/5: LVC fail-closed + hard safety disarm
// ============================================================================

void test_cannot_arm_lvc_critical() {
    auto pre = allGood();
    pre.lvcCritical = true;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_vbat_invalid() {
    auto pre = allGood();
    pre.vbatValid = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_cannot_arm_config_invalid() {
    auto pre = allGood();
    pre.configValid = false;
    auto safety = freshState();
    TEST_ASSERT_FALSE(canArm(pre, safety, DEFAULT_DEBOUNCE));
}

void test_lvc_critical_forces_error_while_armed() {
    auto pre = allGood();
    pre.lvcCritical = true;
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::ERROR, state);
}

void test_vbat_invalid_forces_error_while_armed() {
    auto pre = allGood();
    pre.vbatValid = false;
    SafetyState safety = freshState();
    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::ERROR, state);
}

// ============================================================================
// Round 4: Hot-plug disarm (2g)
// ============================================================================

void test_hot_plug_disarms() {
    // Battery appears while armed with switch high → force disarm
    auto pre = allGood();
    pre.batteryPresent = true;
    SafetyState safety = freshState();
    safety.batteryWasPresent = false; // Battery was NOT present last cycle

    auto state = updateArmState(ArmState::ARMED, pre, safety, 0, defaultCfg());
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

// ============================================================================
// Choke point + throttle mapping (T1, T7)
// ============================================================================

void test_choke_point_all_states() {
    // T1: Every non-ARMED state × any input → output 0
    float input = 0.75f;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, chokeMotorOutput(input, ArmState::BOOT));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, chokeMotorOutput(input, ArmState::DISARMED));
    TEST_ASSERT_EQUAL_FLOAT(input, chokeMotorOutput(input, ArmState::ARMED));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, chokeMotorOutput(input, ArmState::FAILSAFE));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, chokeMotorOutput(input, ArmState::ERROR));
}

void test_throttle_to_dshot_mapping() {
    // T7: output values ∈ {0} ∪ [48, 2047]
    TEST_ASSERT_EQUAL_UINT16(0, throttleToDshot(0.0f, ArmState::ARMED));
    TEST_ASSERT_EQUAL_UINT16(0, throttleToDshot(-0.1f, ArmState::ARMED));
    // Very small positive input → minimum throttle (48 or close to it)
    uint16_t minThrottle = throttleToDshot(0.001f, ArmState::ARMED);
    TEST_ASSERT_GREATER_OR_EQUAL(48, minThrottle);
    TEST_ASSERT_LESS_OR_EQUAL(50, minThrottle);
    TEST_ASSERT_EQUAL_UINT16(2047, throttleToDshot(1.0f, ArmState::ARMED));
    TEST_ASSERT_EQUAL_UINT16(2047, throttleToDshot(1.5f, ArmState::ARMED));

    // Not armed → always 0
    TEST_ASSERT_EQUAL_UINT16(0, throttleToDshot(0.5f, ArmState::DISARMED));
    TEST_ASSERT_EQUAL_UINT16(0, throttleToDshot(1.0f, ArmState::FAILSAFE));
}

void test_throttle_mapping_never_1_to_47() {
    // T7: sweep all normalized values, none should produce 1-47
    for (float n = 0.0f; n <= 1.0f; n += 0.001f) {
        uint16_t val = throttleToDshot(n, ArmState::ARMED);
        if (val != 0) {
            TEST_ASSERT_GREATER_OR_EQUAL(48, val);
        }
    }
}

// ============================================================================
// Runner
// ============================================================================
int main() {
    UNITY_BEGIN();

    // Preconditions
    RUN_TEST(test_can_arm_all_good);
    RUN_TEST(test_cannot_arm_switch_never_low);
    RUN_TEST(test_cannot_arm_spin_stick_nonzero);
    RUN_TEST(test_cannot_arm_no_rc_link);
    RUN_TEST(test_cannot_arm_sensors_unhealthy);
    RUN_TEST(test_cannot_arm_switch_not_high);
    RUN_TEST(test_cannot_arm_invalid_frame);
    RUN_TEST(test_cannot_arm_lq_zero);
    RUN_TEST(test_cannot_arm_insufficient_debounce);
    RUN_TEST(test_cannot_arm_no_battery);
    RUN_TEST(test_cannot_arm_lvc_critical);
    RUN_TEST(test_cannot_arm_vbat_invalid);
    RUN_TEST(test_cannot_arm_config_invalid);
    RUN_TEST(test_preconditions_default_init_all_false);

    // State machine
    RUN_TEST(test_boot_to_disarmed);
    RUN_TEST(test_boot_stays_boot_switch_high);
    RUN_TEST(test_disarmed_to_armed);
    RUN_TEST(test_armed_to_disarmed_switch_low);
    RUN_TEST(test_failsafe_on_link_loss);
    RUN_TEST(test_failsafe_on_lq_zero);
    RUN_TEST(test_failsafe_requires_rearm);
    RUN_TEST(test_failsafe_recovery);
    RUN_TEST(test_sensor_fault_to_error);
    RUN_TEST(test_error_recovery);
    RUN_TEST(test_never_spin_on_boot);

    // Round 4
    RUN_TEST(test_arm_debounce_4_high_1_low_4_high_never_arms);
    RUN_TEST(test_arm_debounce_5_consecutive_arms);
    RUN_TEST(test_lvc_critical_forces_error_while_armed);
    RUN_TEST(test_vbat_invalid_forces_error_while_armed);
    RUN_TEST(test_hot_plug_disarms);
    RUN_TEST(test_choke_point_all_states);
    RUN_TEST(test_throttle_to_dshot_mapping);
    RUN_TEST(test_throttle_mapping_never_1_to_47);

    return UNITY_END();
}
