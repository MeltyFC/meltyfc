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
    TEST_ASSERT_EQUAL(UINT32_MAX, lt.stats().execMinUs);
    TEST_ASSERT_EQUAL(0, lt.stats().execMaxUs);
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

// C7: Execution time tracking
void test_execution_time() {
    lt.startLoop(1000);
    lt.endLoop(1300);     // 300µs of execution
    lt.startLoop(1500);   // 500µs period (start-to-start)
    lt.endLoop(1750);     // 250µs of execution

    // Period stats should reflect 500µs
    TEST_ASSERT_EQUAL(1, lt.stats().totalLoops);
    TEST_ASSERT_EQUAL(500, lt.stats().avgUs);

    // Execution stats: two loops — 300µs then 250µs
    TEST_ASSERT_EQUAL(250, lt.stats().execMinUs);
    TEST_ASSERT_EQUAL(300, lt.stats().execMaxUs);
}

void test_execution_headroom() {
    lt.startLoop(1000);
    lt.endLoop(1200);     // 200µs execution
    lt.startLoop(1500);
    lt.endLoop(1700);     // 200µs execution

    // Budget is 500µs, execution avg ≈ 200µs → headroom ≈ 300µs
    TEST_ASSERT_UINT32_WITHIN(10, 300, lt.headroomUs());
}

void test_execution_no_double_end() {
    lt.startLoop(1000);
    lt.endLoop(1200);
    lt.endLoop(1300);  // Second endLoop should be ignored (no matching start)

    // Should still have only the first measurement
    TEST_ASSERT_EQUAL(200, lt.stats().execMinUs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init_state);
    RUN_TEST(test_single_loop);
    RUN_TEST(test_over_budget_80);
    RUN_TEST(test_over_budget_150);
    RUN_TEST(test_wrap_safe);
    RUN_TEST(test_execution_time);
    RUN_TEST(test_execution_headroom);
    RUN_TEST(test_execution_no_double_end);
    return UNITY_END();
}
