// MeltyFC — LED State Machine Unit Tests

#include "led_beacon.hpp"
#include <cmath>
#include <unity.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

using namespace melty;

static LedConfig defaultCfg() {
    LedConfig cfg;
    cfg.ledCount = 12;
    cfg.arcWidthDeg = 45.0f;
    cfg.beaconColorUp = {0, 255, 0};
    cfg.beaconColorInv = {255, 128, 0};
    cfg.safeColor = {0, 0, 40};
    cfg.armedColor = {255, 0, 0};
    cfg.failsafeColor = {255, 0, 0};
    cfg.lvcWarnColor = {255, 255, 0};
    cfg.lvcCritColor = {255, 0, 0};
    return cfg;
}

// ============================================================================
// State machine
// ============================================================================

void test_led_init_boot() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    TEST_ASSERT_EQUAL(LedState::BOOT, ledSmGetActiveState(sm));
}

void test_led_set_state() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetState(sm, LedState::SAFE, true, 100);
    ledSmSetState(sm, LedState::BOOT, false, 100);
    TEST_ASSERT_EQUAL(LedState::SAFE, ledSmGetActiveState(sm));
}

void test_led_priority() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetState(sm, LedState::SAFE, true, 0);
    ledSmSetState(sm, LedState::ARMED, true, 0);
    ledSmSetState(sm, LedState::FAILSAFE, true, 0);
    // Failsafe is highest priority
    TEST_ASSERT_EQUAL(LedState::FAILSAFE, ledSmGetActiveState(sm));
}

void test_led_failsafe_overrides_armed() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetState(sm, LedState::BOOT, false, 0);
    ledSmSetState(sm, LedState::ARMED, true, 0);
    TEST_ASSERT_EQUAL(LedState::ARMED, ledSmGetActiveState(sm));
    ledSmSetState(sm, LedState::FAILSAFE, true, 0);
    TEST_ASSERT_EQUAL(LedState::FAILSAFE, ledSmGetActiveState(sm));
}

void test_led_lvc_warn_overlay() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    TEST_ASSERT_FALSE(ledSmIsLvcWarn(sm));
    ledSmSetState(sm, LedState::LVC_WARN, true, 0);
    TEST_ASSERT_TRUE(ledSmIsLvcWarn(sm));
}

void test_led_error_blink_code() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetError(sm, 3, 0);
    TEST_ASSERT_EQUAL(LedState::ERROR_BLINK, ledSmGetActiveState(sm));
    TEST_ASSERT_EQUAL_UINT8(3, sm.errorBlinkCode);
}

// ============================================================================
// Effects
// ============================================================================

void test_breathe_returns_valid_range() {
    for (uint32_t t = 0; t < 3000; t += 100) {
        float b = ledBreathe(t, 3000);
        TEST_ASSERT_GREATER_OR_EQUAL(0.0f, b);
        TEST_ASSERT_LESS_OR_EQUAL(1.0f, b);
    }
}

void test_strobe_duty_cycle() {
    // 50% duty @ 1000ms period
    TEST_ASSERT_TRUE(ledStrobe(0, 1000, 0.5f));
    TEST_ASSERT_TRUE(ledStrobe(400, 1000, 0.5f));
    TEST_ASSERT_FALSE(ledStrobe(600, 1000, 0.5f));
    TEST_ASSERT_FALSE(ledStrobe(900, 1000, 0.5f));
}

void test_blink_code_pattern() {
    // 2 blinks: on 200ms, off 200ms, on 200ms, off 200ms, pause 1000ms = 1800ms total
    TEST_ASSERT_TRUE(ledBlinkCode(0, 2, 200, 200, 1000));     // First blink on
    TEST_ASSERT_FALSE(ledBlinkCode(250, 2, 200, 200, 1000));  // First gap
    TEST_ASSERT_TRUE(ledBlinkCode(400, 2, 200, 200, 1000));   // Second blink on
    TEST_ASSERT_FALSE(ledBlinkCode(650, 2, 200, 200, 1000));  // Second gap
    TEST_ASSERT_FALSE(ledBlinkCode(1000, 2, 200, 200, 1000)); // Pause
}

// ============================================================================
// Beacon arc
// ============================================================================

void test_beacon_arc_center() {
    // LED exactly at beacon center — should be in arc
    TEST_ASSERT_TRUE(ledIsInBeaconArc(0.0f, 0.0f, 0.5f));
}

void test_beacon_arc_edge() {
    // LED at edge of arc (within)
    TEST_ASSERT_TRUE(ledIsInBeaconArc(0.4f, 0.0f, 0.5f));
    // Just outside
    TEST_ASSERT_FALSE(ledIsInBeaconArc(0.6f, 0.0f, 0.5f));
}

void test_beacon_arc_wrap() {
    // Beacon near 2π, LED just past 0 — should still be in arc
    float center = 2.0f * M_PI - 0.1f;
    float led = 0.1f;
    TEST_ASSERT_TRUE(ledIsInBeaconArc(led, center, 0.3f));
}

// ============================================================================
// Stationary pattern
// ============================================================================

void test_stationary_safe_produces_color() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetState(sm, LedState::BOOT, false, 0);
    ledSmSetState(sm, LedState::SAFE, true, 0);

    LedConfig cfg = defaultCfg();
    RGB leds[12];
    ledComputeStationary(sm, cfg, 1500, leds); // Mid-breathe

    // At least some LED should have nonzero blue (safe = dim blue breathe)
    bool anyBlue = false;
    for (int i = 0; i < 12; i++) {
        if (leds[i].b > 0)
            anyBlue = true;
    }
    TEST_ASSERT_TRUE(anyBlue);
}

void test_stationary_failsafe_all_same() {
    LedStateMachine sm;
    ledSmInit(sm, 0);
    ledSmSetState(sm, LedState::FAILSAFE, true, 0);

    LedConfig cfg = defaultCfg();
    RGB leds[12];
    ledComputeStationary(sm, cfg, 50, leds); // During strobe-on

    // All LEDs should be the same (all red or all off)
    for (int i = 1; i < 12; i++) {
        TEST_ASSERT_EQUAL_UINT8(leds[0].r, leds[i].r);
        TEST_ASSERT_EQUAL_UINT8(leds[0].g, leds[i].g);
        TEST_ASSERT_EQUAL_UINT8(leds[0].b, leds[i].b);
    }
}

// ============================================================================
// Main
// ============================================================================
int main() {
    UNITY_BEGIN();

    // State machine
    RUN_TEST(test_led_init_boot);
    RUN_TEST(test_led_set_state);
    RUN_TEST(test_led_priority);
    RUN_TEST(test_led_failsafe_overrides_armed);
    RUN_TEST(test_led_lvc_warn_overlay);
    RUN_TEST(test_led_error_blink_code);

    // Effects
    RUN_TEST(test_breathe_returns_valid_range);
    RUN_TEST(test_strobe_duty_cycle);
    RUN_TEST(test_blink_code_pattern);

    // Beacon arc
    RUN_TEST(test_beacon_arc_center);
    RUN_TEST(test_beacon_arc_edge);
    RUN_TEST(test_beacon_arc_wrap);

    // Stationary
    RUN_TEST(test_stationary_safe_produces_color);
    RUN_TEST(test_stationary_failsafe_all_same);

    return UNITY_END();
}
