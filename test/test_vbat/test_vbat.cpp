// MeltyFC — VBAT Filter Tests
// R7-1: Native-testable filter module for battery voltage sag/noise.

#include <unity.h>
#include "vbat_filter.hpp"

using namespace melty;

static VbatFilter filt;

void setUp() {
    filt.init();
}

void tearDown() {}

// --- EMA filtering ---

void test_filter_seeds_on_first_sample() {
    uint16_t result = filt.update(12600, 0);  // 3S nominal = 12.6V
    TEST_ASSERT_EQUAL_UINT16(12600, result);
    TEST_ASSERT_TRUE(filt.isValid());
    TEST_ASSERT_TRUE(filt.isInitialized());
}

void test_filter_smooths_noise() {
    filt.update(12600, 0);   // seed
    filt.update(12500, 1);   // drop 100mV
    uint16_t result = filt.filteredMv();
    // EMA 7/8: (7*12600 + 1*12500) / 8 = 12587
    TEST_ASSERT_UINT16_WITHIN(2, 12587, result);
}

void test_filter_converges_to_stable_input() {
    filt.update(12600, 0);   // seed
    // Feed 50 identical samples — should converge
    for (int i = 1; i <= 50; i++) {
        filt.update(12000, i);
    }
    TEST_ASSERT_UINT16_WITHIN(10, 12000, filt.filteredMv());
}

// --- Range validation ---

void test_filter_rejects_below_min() {
    filt.update(12600, 0);   // seed with valid
    filt.update(500, 1);     // below 2000mV min
    TEST_ASSERT_FALSE(filt.isValid());
    TEST_ASSERT_EQUAL_UINT16(12600, filt.filteredMv());  // unchanged
}

void test_filter_rejects_above_max() {
    filt.update(12600, 0);
    filt.update(30000, 1);   // above 26000mV max
    TEST_ASSERT_FALSE(filt.isValid());
}

void test_filter_not_valid_before_first_sample() {
    TEST_ASSERT_FALSE(filt.isValid());
    TEST_ASSERT_FALSE(filt.isInitialized());
}

// --- Arming grace window ---

void test_grace_prevents_false_sag() {
    filt.update(12600, 0);   // seed
    filt.onArm(100);         // arm at t=100ms

    // Simulate spin-up sag: voltage drops 400mV
    for (int i = 0; i < 10; i++) {
        filt.update(12200, 100 + i * 10);
    }

    // During 500ms grace window, filtered should not drop below pre-arm
    TEST_ASSERT_TRUE(filt.filteredMv() >= 12600);
}

void test_grace_expires_after_window() {
    filt.update(12600, 0);
    filt.onArm(100);

    // Past the 500ms grace window
    for (int i = 0; i < 100; i++) {
        filt.update(12200, 700 + i * 10);
    }

    // Grace expired — filter should track the lower voltage
    TEST_ASSERT_TRUE(filt.filteredMv() < 12600);
}

void test_disarm_ends_grace_immediately() {
    filt.update(12600, 0);
    filt.onArm(100);
    filt.onDisarm();

    // Sag should now be tracked (grace ended)
    for (int i = 0; i < 20; i++) {
        filt.update(12200, 110 + i * 10);
    }
    TEST_ASSERT_TRUE(filt.filteredMv() < 12600);
}

// --- Custom config ---

void test_custom_config_range() {
    VbatFilterConfig cfg = VBAT_FILTER_DEFAULT;
    cfg.minValidMv = 6000;   // 2S minimum
    cfg.maxValidMv = 9000;   // 2S maximum

    VbatFilter f2;
    f2.init(cfg);
    f2.update(7400, 0);
    TEST_ASSERT_TRUE(f2.isValid());

    f2.update(12600, 1);  // 3S voltage — out of 2S range
    TEST_ASSERT_FALSE(f2.isValid());
}

// --- Runner ---

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_filter_seeds_on_first_sample);
    RUN_TEST(test_filter_smooths_noise);
    RUN_TEST(test_filter_converges_to_stable_input);
    RUN_TEST(test_filter_rejects_below_min);
    RUN_TEST(test_filter_rejects_above_max);
    RUN_TEST(test_filter_not_valid_before_first_sample);
    RUN_TEST(test_grace_prevents_false_sag);
    RUN_TEST(test_grace_expires_after_window);
    RUN_TEST(test_disarm_ends_grace_immediately);
    RUN_TEST(test_custom_config_range);

    return UNITY_END();
}
