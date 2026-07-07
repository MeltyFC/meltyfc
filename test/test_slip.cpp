// MeltyFC — Slip Detection Unit Tests

#include <unity.h>
#include "../src/slip.hpp"
#include <cmath>

using namespace melty;

void test_erpm_to_mech_rpm() {
    // 14 poles → 7 pole pairs → mech = erpm / 7
    float rpm = erpmToMechRpm(21000, 14);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3000.0f, rpm);
}

void test_erpm_zero_poles() {
    float rpm = erpmToMechRpm(21000, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rpm);
}

void test_wheel_implied_omega() {
    SlipConfig cfg = {5.0f, 40.0f, 85.0f, 14, 25.0f, 300};
    // motor 3000 RPM, ratio 5:1 → wheel 600 RPM
    // omega = 600 * π * 40 / (60 * 85) = 75398 / 5100 ≈ 14.78 rad/s
    float omega = wheelImpliedOmega(3000.0f, cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 14.78f, omega);
}

void test_slip_no_slip() {
    // Accel omega matches wheel omega → 0% slip
    float slip = computeSlipPct(100.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, slip);
}

void test_slip_full_slip() {
    // Accel omega = 0 (bot not spinning) but wheels turning → 100% slip
    float slip = computeSlipPct(0.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, slip);
}

void test_slip_partial() {
    // Accel omega = 75 vs wheel omega = 100 → 25% slip
    float slip = computeSlipPct(75.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, slip);
}

void test_slip_near_zero_omega() {
    // Near-zero wheel omega shouldn't divide by zero
    float slip = computeSlipPct(0.0f, 0.001f);
    TEST_ASSERT(slip >= 0.0f);
    TEST_ASSERT(slip <= 100.0f);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_erpm_to_mech_rpm);
    RUN_TEST(test_erpm_zero_poles);
    RUN_TEST(test_wheel_implied_omega);
    RUN_TEST(test_slip_no_slip);
    RUN_TEST(test_slip_full_slip);
    RUN_TEST(test_slip_partial);
    RUN_TEST(test_slip_near_zero_omega);
    return UNITY_END();
}
