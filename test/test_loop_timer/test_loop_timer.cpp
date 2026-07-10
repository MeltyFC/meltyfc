#include <unity.h>
#include "loop_timer.hpp"

using namespace melty;

static LoopTimer lt;

void setUp() { lt.init(500); }
void tearDown() {}

void test_init_state() {
    TEST_ASSERT_EQUAL(0, lt.stats().totalLoops);
    TEST_ASSERT_EQUAL(UINT32_MAX, lt.stats().minUs);
    TEST_ASSERT_EQUAL(0, lt.stats().maxUs);
}

void test_single_loop() {
    lt.startLoop(1000);
    lt.startLoop(1450);  // 450µs loop
    TEST_ASSERT_EQUAL(1, lt.stats().totalLoops);
    TEST_ASSERT_EQUAL(450, lt.stats().minUs);
    TEST_ASSERT_EQUAL(450, lt.stats().maxUs);
}

void test_over_budget_80() {
    lt.startLoop(1000);
    lt.startLoop(1410);  // 410µs = 82% of 500µs budget
    TEST_ASSERT_EQUAL(1, lt.stats().overBudget80);
    TEST_ASSERT_EQUAL(0, lt.stats().overBudget150);
}

void test_over_budget_150() {
    lt.startLoop(1000);
    lt.startLoop(1800);  // 800µs = 160% of 500µs budget
    TEST_ASSERT_EQUAL(1, lt.stats().overBudget80);
    TEST_ASSERT_EQUAL(1, lt.stats().overBudget150);
}

void test_wrap_safe() {
    // Unsigned wrap: 0xFFFFFF00 -> 0x00000100 = 0x200 = 512µs
    lt.startLoop(0xFFFFFF00U);
    lt.startLoop(0x00000100U);
    TEST_ASSERT_UINT32_WITHIN(10, 512, lt.stats().avgUs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_single_loop);
    RUN_TEST(test_over_budget_80);
    RUN_TEST(test_over_budget_150);
    RUN_TEST(test_wrap_safe);
    return UNITY_END();
}
