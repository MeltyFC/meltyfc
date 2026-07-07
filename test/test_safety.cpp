// MeltyFC — Safety & Arming Unit Tests

#include <unity.h>
#include "safety.hpp"

using namespace melty;

static ArmPreconditions allGood() {
    return {true, true, true, true, true};
}

// ============================================================================
// Arming preconditions
// ============================================================================

void test_can_arm_all_good() {
    TEST_ASSERT_TRUE(canArm(allGood()));
}

void test_cannot_arm_switch_never_low() {
    auto pre = allGood();
    pre.armSwitchLow = false;
    TEST_ASSERT_FALSE(canArm(pre));
}

void test_cannot_arm_spin_stick_nonzero() {
    auto pre = allGood();
    pre.spinStickZero = false;
    TEST_ASSERT_FALSE(canArm(pre));
}

void test_cannot_arm_no_rc_link() {
    auto pre = allGood();
    pre.rcLinkHealthy = false;
    TEST_ASSERT_FALSE(canArm(pre));
}

void test_cannot_arm_sensors_unhealthy() {
    auto pre = allGood();
    pre.sensorsHealthy = false;
    TEST_ASSERT_FALSE(canArm(pre));
}

void test_cannot_arm_switch_not_high() {
    auto pre = allGood();
    pre.armSwitchCurrentlyHigh = false;
    TEST_ASSERT_FALSE(canArm(pre));
}

// ============================================================================
// State machine transitions
// ============================================================================

void test_boot_to_disarmed() {
    ArmPreconditions pre = {true, true, true, true, false};  // Switch LOW
    auto state = updateArmState(ArmState::BOOT, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_boot_stays_boot_switch_high() {
    ArmPreconditions pre = {false, true, true, true, true};  // Switch HIGH, never seen LOW
    auto state = updateArmState(ArmState::BOOT, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::BOOT, state);
}

void test_disarmed_to_armed() {
    auto state = updateArmState(ArmState::DISARMED, allGood(), 0, 500);
    TEST_ASSERT_EQUAL(ArmState::ARMED, state);
}

void test_armed_to_disarmed_switch_low() {
    ArmPreconditions pre = {true, true, true, true, false};  // Switch LOW
    auto state = updateArmState(ArmState::ARMED, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_failsafe_on_link_loss() {
    auto state = updateArmState(ArmState::ARMED, allGood(), 600, 500);  // 600ms > 500ms
    TEST_ASSERT_EQUAL(ArmState::FAILSAFE, state);
}

void test_failsafe_requires_rearm() {
    // Link restored but switch still HIGH → stay in failsafe
    ArmPreconditions pre = {true, true, true, true, true};
    auto state = updateArmState(ArmState::FAILSAFE, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::FAILSAFE, state);
}

void test_failsafe_recovery() {
    // Link restored + switch LOW → back to DISARMED (must re-arm)
    ArmPreconditions pre = {true, true, true, true, false};
    auto state = updateArmState(ArmState::FAILSAFE, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_sensor_fault_to_error() {
    ArmPreconditions pre = {true, true, true, false, true};  // Sensors unhealthy
    auto state = updateArmState(ArmState::ARMED, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::ERROR, state);
}

void test_error_recovery() {
    ArmPreconditions pre = {true, true, true, true, false};  // Sensors back + switch LOW
    auto state = updateArmState(ArmState::ERROR, pre, 0, 500);
    TEST_ASSERT_EQUAL(ArmState::DISARMED, state);
}

void test_never_spin_on_boot() {
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::BOOT));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::DISARMED));
    TEST_ASSERT_TRUE(motorsAllowed(ArmState::ARMED));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::FAILSAFE));
    TEST_ASSERT_FALSE(motorsAllowed(ArmState::ERROR));
}

int main() {
    UNITY_BEGIN();

    // Preconditions
    RUN_TEST(test_can_arm_all_good);
    RUN_TEST(test_cannot_arm_switch_never_low);
    RUN_TEST(test_cannot_arm_spin_stick_nonzero);
    RUN_TEST(test_cannot_arm_no_rc_link);
    RUN_TEST(test_cannot_arm_sensors_unhealthy);
    RUN_TEST(test_cannot_arm_switch_not_high);

    // State machine
    RUN_TEST(test_boot_to_disarmed);
    RUN_TEST(test_boot_stays_boot_switch_high);
    RUN_TEST(test_disarmed_to_armed);
    RUN_TEST(test_armed_to_disarmed_switch_low);
    RUN_TEST(test_failsafe_on_link_loss);
    RUN_TEST(test_failsafe_requires_rearm);
    RUN_TEST(test_failsafe_recovery);
    RUN_TEST(test_sensor_fault_to_error);
    RUN_TEST(test_error_recovery);
    RUN_TEST(test_never_spin_on_boot);

    return UNITY_END();
}
