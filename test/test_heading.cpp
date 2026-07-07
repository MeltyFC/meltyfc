// MeltyFC — Heading Engine Unit Tests
// Tests differential omega, phase integration, motor output, trim, RPM hold.

#include <unity.h>
#include "heading.hpp"
#include <cmath>

using namespace melty;

// ============================================================================
// Differential omega
// ============================================================================

void test_omega_differential_basic() {
    // Known case: outer=100g, inner=50g, dr=0.013m (13mm)
    // omega = sqrt((100-50)*9.80665 / 0.013) = sqrt(37717.9) ≈ 194.2 rad/s
    float omega = computeOmegaDifferential(100.0f, 50.0f, 0.013f);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 194.2f, omega);
}

void test_omega_differential_hit_rejection() {
    // Hit adds 200g common-mode to BOTH sensors — omega should not change
    float omegaClean = computeOmegaDifferential(100.0f, 50.0f, 0.013f);
    float omegaHit   = computeOmegaDifferential(300.0f, 250.0f, 0.013f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, omegaClean, omegaHit);
}

void test_omega_differential_negative_clamp() {
    // Transient: inner > outer — should return 0, not NaN
    float omega = computeOmegaDifferential(50.0f, 100.0f, 0.013f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, omega);
}

void test_omega_differential_zero_dr() {
    float omega = computeOmegaDifferential(100.0f, 50.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, omega);
}

// ============================================================================
// Slew limiting
// ============================================================================

void test_slew_limit_within_bounds() {
    float result = slewLimitOmega(100.0f, 95.0f, 200.0f, 0.001f);
    // Delta = 5, max delta at 200 rad/s/s * 0.001s = 0.2 → clamp to 95.2
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 95.2f, result);
}

void test_slew_limit_no_clamp_needed() {
    float result = slewLimitOmega(100.0f, 99.99f, 200.0f, 0.001f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, result);
}

// ============================================================================
// Phase integration
// ============================================================================

void test_phase_integration_basic() {
    // omega = 2π rad/s (1 rev/s), dt = 0.25s → phase advances π/2
    float phase = integratePhase(0.0f, 2.0f * M_PI, 0.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, M_PI / 2.0f, phase);
}

void test_phase_integration_wraps() {
    // Start at 6.0 rad, advance 1.0 rad → should wrap past 2π
    float phase = integratePhase(6.0f, 100.0f, 0.01f);
    TEST_ASSERT(phase >= 0.0f);
    TEST_ASSERT(phase < 2.0f * M_PI);
}

// ============================================================================
// Translation
// ============================================================================

void test_translation_angle_forward() {
    // Stick full forward (Y=1, X=0) → angle = 0
    float angle = computeTranslationAngle(0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, angle);
}

void test_translation_angle_right() {
    // Stick full right (X=1, Y=0) → angle = π/2
    float angle = computeTranslationAngle(1.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, M_PI / 2.0f, angle);
}

void test_translation_magnitude_deadband() {
    float mag = computeTranslationMagnitude(0.03f, 0.03f, 0.05f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, mag);
}

void test_translation_magnitude_full() {
    float mag = computeTranslationMagnitude(0.0f, 1.0f, 0.05f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, mag);
}

// ============================================================================
// Motor output — N=2 (0°, 180°)
// ============================================================================

void test_motor_output_spin_only() {
    // No translation — all motors get spin throttle
    float out = computeMotorOutput(0.0f, 0.0f, 0.0f, 0.5f, 0.524f, 0.9f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, out);
}

void test_motor_output_boost_in_window() {
    // Motor at 0°, translation at 0° → motor IS in the window → boosted
    float out = computeMotorOutput(0.0f, 0.0f, 1.0f, 0.5f, 0.524f, 0.9f);
    TEST_ASSERT(out > 0.5f);
    TEST_ASSERT(out <= 0.9f);
}

void test_motor_output_opposing_window() {
    // Motor at π, translation at 0° → motor in opposing window → reduced
    float out = computeMotorOutput(M_PI, 0.0f, 1.0f, 0.5f, 0.524f, 0.9f);
    TEST_ASSERT(out < 0.5f);
    TEST_ASSERT(out >= 0.0f);
}

void test_motor_output_outside_both_windows() {
    // Motor at π/2, translation at 0° → neither window → spin only
    float out = computeMotorOutput(M_PI / 2.0f, 0.0f, 1.0f, 0.5f, 0.524f, 0.9f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, out);
}

void test_motor_output_clamp_to_cap() {
    // Even with full boost, output should never exceed throttleCap
    float out = computeMotorOutput(0.0f, 0.0f, 1.0f, 0.85f, 1.0f, 0.9f);
    TEST_ASSERT(out <= 0.9f);
}

// ============================================================================
// Inversion
// ============================================================================

void test_inversion_normal() {
    float angle = applyInversion(1.0f, false);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, angle);
}

void test_inversion_inverted() {
    float angle = applyInversion(1.0f, true);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, angle);
}

// ============================================================================
// Trim
// ============================================================================

void test_trim_deadband() {
    TrimConfig cfg = {15.0f, 360.0f, 0.3f};
    float rate = computeTrimRate(0.01f, cfg);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rate);
}

void test_trim_full_deflection() {
    TrimConfig cfg = {15.0f, 360.0f, 0.0f};  // Linear (no expo)
    float rate = computeTrimRate(1.0f, cfg);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 360.0f, rate);
}

void test_trim_negative() {
    TrimConfig cfg = {15.0f, 360.0f, 0.0f};
    float rate = computeTrimRate(-1.0f, cfg);
    TEST_ASSERT(rate < 0.0f);
}

// ============================================================================
// RPM Hold
// ============================================================================

void test_rpm_hold_disabled() {
    RpmHoldConfig cfg = {false, 2800.0f, 0.002f, 0.5f};
    float throttle = computeRpmHold(2500.0f, 2800.0f, 0.6f, cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, throttle);  // Passthrough
}

void test_rpm_hold_below_target() {
    RpmHoldConfig cfg = {true, 2800.0f, 0.002f, 0.5f};
    float throttle = computeRpmHold(2500.0f, 2800.0f, 0.6f, cfg);
    // Error = 300, correction = 0.002*300 = 0.6, output = 0.5 + 0.6 = 1.0 (clamped)
    TEST_ASSERT(throttle > 0.5f);
}

void test_rpm_hold_at_target() {
    RpmHoldConfig cfg = {true, 2800.0f, 0.002f, 0.5f};
    float throttle = computeRpmHold(2800.0f, 2800.0f, 0.6f, cfg);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, throttle);  // Feedforward only
}

// ============================================================================
// Hit detection
// ============================================================================

void test_hit_detected() {
    TEST_ASSERT_TRUE(detectHit(300.0f, 100.0f, 50.0f));
}

void test_hit_not_detected() {
    TEST_ASSERT_FALSE(detectHit(120.0f, 100.0f, 50.0f));
}

// ============================================================================
// RPM/omega conversion
// ============================================================================

void test_rpm_to_omega() {
    float omega = rpmToOmega(60.0f);  // 60 RPM = 1 rev/s = 2π rad/s
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f * M_PI, omega);
}

void test_omega_to_rpm() {
    float rpm = omegaToRpm(2.0f * M_PI);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, rpm);
}

// ============================================================================
// Test runner
// ============================================================================

int main() {
    UNITY_BEGIN();

    // Differential omega
    RUN_TEST(test_omega_differential_basic);
    RUN_TEST(test_omega_differential_hit_rejection);
    RUN_TEST(test_omega_differential_negative_clamp);
    RUN_TEST(test_omega_differential_zero_dr);

    // Slew
    RUN_TEST(test_slew_limit_within_bounds);
    RUN_TEST(test_slew_limit_no_clamp_needed);

    // Phase
    RUN_TEST(test_phase_integration_basic);
    RUN_TEST(test_phase_integration_wraps);

    // Translation
    RUN_TEST(test_translation_angle_forward);
    RUN_TEST(test_translation_angle_right);
    RUN_TEST(test_translation_magnitude_deadband);
    RUN_TEST(test_translation_magnitude_full);

    // Motor output
    RUN_TEST(test_motor_output_spin_only);
    RUN_TEST(test_motor_output_boost_in_window);
    RUN_TEST(test_motor_output_opposing_window);
    RUN_TEST(test_motor_output_outside_both_windows);
    RUN_TEST(test_motor_output_clamp_to_cap);

    // Inversion
    RUN_TEST(test_inversion_normal);
    RUN_TEST(test_inversion_inverted);

    // Trim
    RUN_TEST(test_trim_deadband);
    RUN_TEST(test_trim_full_deflection);
    RUN_TEST(test_trim_negative);

    // RPM hold
    RUN_TEST(test_rpm_hold_disabled);
    RUN_TEST(test_rpm_hold_below_target);
    RUN_TEST(test_rpm_hold_at_target);

    // Hit detection
    RUN_TEST(test_hit_detected);
    RUN_TEST(test_hit_not_detected);

    // Conversions
    RUN_TEST(test_rpm_to_omega);
    RUN_TEST(test_omega_to_rpm);

    return UNITY_END();
}
