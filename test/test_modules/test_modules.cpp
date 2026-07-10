// MeltyFC — Creep, Resync, Hit Logger, LVC, Blackbox Unit Tests

#include "blackbox.hpp"
#include "creep.hpp"
#include "hitlog.hpp"
#include "lvc.hpp"
#include "resync.hpp"
#include <cmath>
#include <cstring>
#include <unity.h>

using namespace melty;

// ============================================================================
// Creep mode
// ============================================================================

void test_creep_enters_below_threshold() {
    CreepState state = {false, false};
    CreepConfig cfg = {200, 50, 3};
    TEST_ASSERT_TRUE(creepUpdateState(state, 100.0f, false, cfg));
    TEST_ASSERT_TRUE(state.active);
}

void test_creep_stays_with_hysteresis() {
    CreepState state = {true, false};
    CreepConfig cfg = {200, 50, 3};
    // At 220 RPM — above threshold but below threshold+hysteresis (250)
    TEST_ASSERT_TRUE(creepUpdateState(state, 220.0f, false, cfg));
    TEST_ASSERT_TRUE(state.active);
}

void test_creep_exits_above_hysteresis() {
    CreepState state = {true, false};
    CreepConfig cfg = {200, 50, 3};
    // At 260 RPM — above threshold+hysteresis
    TEST_ASSERT_FALSE(creepUpdateState(state, 260.0f, false, cfg));
    TEST_ASSERT_FALSE(state.active);
}

void test_creep_force_switch() {
    CreepState state = {false, false};
    CreepConfig cfg = {200, 50, 3};
    TEST_ASSERT_TRUE(creepUpdateState(state, 3000.0f, true, cfg));
    TEST_ASSERT_TRUE(state.forcedBySwitch);
}

void test_creep_output_forward() {
    float out[4] = {0};
    creepComputeOutput(0.0f, 1.0f, 1.0f, 2, out);
    // Both motors forward
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[1]);
}

void test_creep_output_turn() {
    float out[4] = {0};
    creepComputeOutput(1.0f, 0.0f, 1.0f, 2, out);
    // S2: Forward-only creep (DShot has no reverse without 3D mode)
    // Left wheel gets thrust, right wheel stops → pivot steering
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, out[1]); // Clamped to 0, not -1
}

void test_creep_output_3motor_third_passive() {
    float out[4] = {0};
    creepComputeOutput(0.0f, 1.0f, 1.0f, 3, out);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[2]);
}

void test_creep_output_clamped() {
    float out[4] = {0};
    creepComputeOutput(1.0f, 1.0f, 0.5f, 2, out);
    // left = 1+1=2 → clamped to 0.5
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, out[0]);
}

// ============================================================================
// Heading re-sync
// ============================================================================

void test_resync_init() {
    ResyncState state;
    resyncInit(state);
    TEST_ASSERT_FALSE(state.held);
    TEST_ASSERT_EQUAL_UINT32(0, state.sampleCount);
}

void test_resync_hold_accumulates() {
    ResyncState state;
    resyncInit(state);
    ResyncConfig cfg = {0.3f, 100};

    // Press switch, point stick forward
    resyncUpdate(state, true, 0.0f, 1.0f, 100, cfg);
    resyncUpdate(state, true, 0.0f, 1.0f, 200, cfg);
    TEST_ASSERT_TRUE(state.held);
    TEST_ASSERT_EQUAL_UINT32(2, state.sampleCount);
}

void test_resync_release_returns_angle() {
    ResyncState state;
    resyncInit(state);
    ResyncConfig cfg = {0.3f, 100};

    // Hold and point right (stickX=1, stickY=0 → angle ≈ π/2)
    resyncUpdate(state, true, 1.0f, 0.0f, 100, cfg);
    resyncUpdate(state, true, 1.0f, 0.0f, 200, cfg);

    // Release
    float offset = resyncUpdate(state, false, 0.0f, 0.0f, 300, cfg);
    // Should return ~π/2 (atan2(1,0))
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.5708f, offset);
}

void test_resync_below_threshold_cancels() {
    ResyncState state;
    resyncInit(state);
    ResyncConfig cfg = {0.3f, 100};

    // Hold with tiny deflection (below minDeflection=0.3)
    resyncUpdate(state, true, 0.1f, 0.0f, 100, cfg);
    float offset = resyncUpdate(state, false, 0.0f, 0.0f, 200, cfg);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, offset); // Cancelled
}

void test_resync_stick_angle() {
    // Forward = atan2(0, 1) = 0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, resyncStickAngle(0.0f, 1.0f));
    // Right = atan2(1, 0) = π/2
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5708f, resyncStickAngle(1.0f, 0.0f));
}

void test_resync_stick_magnitude() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, resyncStickMagnitude(1.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, resyncStickMagnitude(0.0f, 0.0f));
    // Diagonal: sqrt(0.5² + 0.5²) ≈ 0.707
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.707f, resyncStickMagnitude(0.5f, 0.5f));
}

// ============================================================================
// Hit logger
// ============================================================================

void test_hitlog_init_empty() {
    HitLogger log;
    hitLogInit(log);
    TEST_ASSERT_EQUAL_UINT16(0, hitLogCount(log));
    TEST_ASSERT_NULL(hitLogLatest(log));
}

void test_hitlog_record_and_retrieve() {
    HitLogger log;
    hitLogInit(log);
    hitLogRecord(log, 1000, 150.0f, 300.0f);
    TEST_ASSERT_EQUAL_UINT16(1, hitLogCount(log));
    const HitRecord* latest = hitLogLatest(log);
    TEST_ASSERT_NOT_NULL(latest);
    TEST_ASSERT_EQUAL_FLOAT(150.0f, latest->peakG);
}

void test_hitlog_max_g_tracked() {
    HitLogger log;
    hitLogInit(log);
    hitLogRecord(log, 100, 50.0f, 200.0f);
    hitLogRecord(log, 200, 200.0f, 300.0f);
    hitLogRecord(log, 300, 100.0f, 250.0f);
    TEST_ASSERT_EQUAL_FLOAT(200.0f, hitLogMaxG(log));
}

void test_hitlog_wraps() {
    HitLogger log;
    hitLogInit(log);
    // Fill beyond capacity
    for (int i = 0; i < HIT_LOG_SIZE + 5; i++) {
        hitLogRecord(log, i * 100, (float)i, 100.0f);
    }
    TEST_ASSERT_EQUAL_UINT16(HIT_LOG_SIZE + 5, hitLogCount(log));
    const HitRecord* latest = hitLogLatest(log);
    TEST_ASSERT_NOT_NULL(latest);
    TEST_ASSERT_EQUAL_FLOAT((float)(HIT_LOG_SIZE + 4), latest->peakG);
}

void test_hitlog_format() {
    HitLogger log;
    hitLogInit(log);
    hitLogRecord(log, 5000, 175.5f, 293.2f);
    char buf[256];
    int n = hitLogFormat(log, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "175.5"));
}

// ============================================================================
// LVC
// ============================================================================

void test_lvc_auto_detect_3s() {
    TEST_ASSERT_EQUAL_UINT8(3, lvcAutoDetectCells(11.1f));
}

void test_lvc_auto_detect_4s() {
    TEST_ASSERT_EQUAL_UINT8(4, lvcAutoDetectCells(14.8f));
}

void test_lvc_auto_detect_too_low() {
    TEST_ASSERT_EQUAL_UINT8(0, lvcAutoDetectCells(1.0f));
}

// Finding 3: LVC fails closed when cell count unknown
void test_lvc_update_sensor_fault() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 0};              // Auto-detect
    LvcLevel level = lvcUpdate(state, 1.0f, cfg); // Too low to detect
    TEST_ASSERT_EQUAL(LvcLevel::SENSOR_FAULT, level);
}

void test_lvc_update_cell_count_unknown() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 0}; // Auto-detect
    // 27V is out of range for auto-detect (>6S)
    LvcLevel level = lvcUpdate(state, 27.0f, cfg);
    TEST_ASSERT_EQUAL(LvcLevel::CELL_COUNT_UNKNOWN, level);
}

void test_lvc_update_ok() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 3};
    LvcLevel level = lvcUpdate(state, 11.4f, cfg);
    TEST_ASSERT_EQUAL(LvcLevel::OK, level);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.8f, state.cellVoltage);
}

void test_lvc_update_warn() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 3};
    LvcLevel level = lvcUpdate(state, 9.6f, cfg); // 3.2V/cell
    TEST_ASSERT_EQUAL(LvcLevel::WARN, level);
}

void test_lvc_update_critical() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 3};
    LvcLevel level = lvcUpdate(state, 8.7f, cfg); // 2.9V/cell
    TEST_ASSERT_EQUAL(LvcLevel::CRITICAL, level);
    TEST_ASSERT_TRUE(state.spinDownActive);
}

void test_lvc_spindown_ramp() {
    // B5/I-34: Hard cut, no ramp. spinDownActive → immediate zero.
    LvcState state = {3, 0, 0, LvcLevel::CRITICAL, true};
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, lvcSpinDownThrottle(state, 0.75f, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, lvcSpinDownThrottle(state, 0.75f, 1000));
    // Not active → pass through
    LvcState stateOk = {3, 0, 0, LvcLevel::OK, false};
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.75f, lvcSpinDownThrottle(stateOk, 0.75f, 0));
}

void test_lvc_auto_detect_sticks() {
    LvcState state = {};
    LvcConfig cfg = {3.3f, 3.0f, 0.05f, 0}; // Auto-detect

    // First reading at 11.1V → auto-detect 3S
    lvcUpdate(state, 11.1f, cfg);
    TEST_ASSERT_EQUAL_UINT8(3, state.detectedCells);

    // Voltage drops but cell count stays
    lvcUpdate(state, 9.0f, cfg);
    TEST_ASSERT_EQUAL_UINT8(3, state.detectedCells); // Doesn't re-detect
}

// ============================================================================
// Blackbox
// ============================================================================

void test_blackbox_init() {
    BlackboxState state;
    blackboxInit(state, 8 * 1024 * 1024, 4096); // 8MB, 4K sectors
    TEST_ASSERT_EQUAL_UINT32(0, state.writeOffset);
    TEST_ASSERT_FALSE(state.wrapped);
}

void test_blackbox_capacity() {
    BlackboxState state;
    blackboxInit(state, 8 * 1024 * 1024, 4096);
    uint32_t cap = blackboxCapacity(state);
    // 8MB / sizeof(BlackboxRecord)
    TEST_ASSERT_EQUAL_UINT32(8 * 1024 * 1024 / sizeof(BlackboxRecord), cap);
}

void test_blackbox_next_offset() {
    BlackboxState state;
    blackboxInit(state, 280, 4096); // Small flash for testing (10 records at 28 bytes)
    uint32_t off1 = blackboxNextOffset(state);
    TEST_ASSERT_EQUAL_UINT32(0, off1);
    uint32_t off2 = blackboxNextOffset(state);
    TEST_ASSERT_EQUAL_UINT32(sizeof(BlackboxRecord), off2);
}

void test_blackbox_wraps() {
    BlackboxState state;
    blackboxInit(state, 84, 4096); // 3 records fit (84 / 28 = 3)

    (void)blackboxNextOffset(state); // offset=0, writeOffset→28
    TEST_ASSERT_FALSE(state.wrapped);
    (void)blackboxNextOffset(state); // offset=28, writeOffset→56
    TEST_ASSERT_FALSE(state.wrapped);
    (void)blackboxNextOffset(state);        // offset=56, writeOffset→84
    TEST_ASSERT_FALSE(state.wrapped); // Buffer full but not yet wrapped
    // 4th call triggers wrap — writeOffset=84 >= usableSize=84
    (void)blackboxNextOffset(state); // wraps → offset=0, writeOffset→28
    TEST_ASSERT_TRUE(state.wrapped);
}

void test_blackbox_stored() {
    BlackboxState state;
    blackboxInit(state, 280, 4096); // 10 records

    TEST_ASSERT_EQUAL_UINT32(0, blackboxStored(state));
    (void)blackboxNextOffset(state);
    TEST_ASSERT_EQUAL_UINT32(1, blackboxStored(state));
    (void)blackboxNextOffset(state);
    TEST_ASSERT_EQUAL_UINT32(2, blackboxStored(state));
}

void test_blackbox_read_offset() {
    BlackboxState state;
    blackboxInit(state, 280, 4096);

    (void)blackboxNextOffset(state); // offset 0
    (void)blackboxNextOffset(state); // offset 28
    (void)blackboxNextOffset(state); // offset 56

    uint32_t offset;
    // Most recent (N=0) should be at 48
    TEST_ASSERT_TRUE(blackboxReadOffset(state, 0, &offset));
    TEST_ASSERT_EQUAL_UINT32(2 * sizeof(BlackboxRecord), offset);

    // N=1 should be at sizeof(BlackboxRecord)
    TEST_ASSERT_TRUE(blackboxReadOffset(state, 1, &offset));
    TEST_ASSERT_EQUAL_UINT32(sizeof(BlackboxRecord), offset);

    // N=3 should fail (only 3 records)
    TEST_ASSERT_FALSE(blackboxReadOffset(state, 3, &offset));
}

void test_blackbox_format() {
    BlackboxRecord rec = {BLACKBOX_RECORD_MAGIC, 0, 1000, 293.2f, 1.5f, 11.1f, 5.0f, 0, 1, 1, 0};
    char buf[256];
    int n = blackboxFormatRecord(rec, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "1000"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "293.20"));
}

void test_blackbox_format_header() {
    char buf[256];
    int n = blackboxFormatHeader(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "timestamp_ms"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "omega_rad_s"));
}

// R16-5: Blackbox check-defer
void test_blackbox_defers_on_busy() {
    BlackboxDeferState defer;
    blackboxDeferInit(defer);
    TEST_ASSERT_FALSE(defer.hasPending);

    BlackboxRecord rec = {BLACKBOX_RECORD_MAGIC, 0, 1000, 100.0f, 0.5f, 11.1f, 5.0f, 0, 0, 1, 0};

    // Flash not busy → no defer
    TEST_ASSERT_FALSE(blackboxDeferIfBusy(defer, rec, false));
    TEST_ASSERT_FALSE(defer.hasPending);

    // Flash busy → defer
    TEST_ASSERT_TRUE(blackboxDeferIfBusy(defer, rec, true));
    TEST_ASSERT_TRUE(defer.hasPending);

    // Retry deferred → get the record back
    BlackboxRecord out = {};
    TEST_ASSERT_TRUE(blackboxRetryDeferred(defer, out));
    TEST_ASSERT_EQUAL_UINT32(1000, out.timestampMs);
    TEST_ASSERT_FALSE(defer.hasPending); // Cleared after retry

    // No pending → retry returns false
    TEST_ASSERT_FALSE(blackboxRetryDeferred(defer, out));
}

void test_resync_wrap_around_backward() {
    // THE BUG: pointing backward (±180°), arithmetic mean of angles near ±π
    // averages to ~0 (forward) instead of backward. Circular mean fixes this.
    ResyncState state;
    resyncInit(state);
    ResyncConfig cfg = {0.3f, 100};

    // Point stick backward-left and backward-right (straddling ±π wrap)
    // stickX=-0.5, stickY=-0.866 → angle ≈ -2.618 rad (−150°)
    // stickX=0.5, stickY=-0.866 → angle ≈ 2.618 rad (+150°)
    // Circular mean should be π (backward), not 0 (forward)
    resyncUpdate(state, true, -0.5f, -0.866f, 100, cfg);
    resyncUpdate(state, true, 0.5f, -0.866f, 200, cfg);

    float offset = resyncUpdate(state, false, 0.0f, 0.0f, 300, cfg);

    // Should be near ±π (backward), NOT near 0 (forward)
    // Allow wide tolerance — key assertion is |offset| > π/2
    TEST_ASSERT_GREATER_THAN(1.5f, fabsf(offset)); // Must be > π/2
}

// ============================================================================
// Main
// ============================================================================
// Phase G/B2: Cell ambiguity suite
void test_cell_detect_12_4v_ambiguous() {
    // 12.4V is in the 3S/4S overlap window — must return 0 (ambiguous)
    TEST_ASSERT_EQUAL_UINT8(0, lvcAutoDetectCells(13.2f)); // In 3S/4S overlap (13.05-13.5)
}
void test_cell_detect_9_0v_ambiguous() {
    // 9.0V is in the 2S/3S overlap window
    TEST_ASSERT_EQUAL_UINT8(0, lvcAutoDetectCells(9.0f));
}
void test_cell_detect_11_1v_unambiguous_3s() {
    // 11.1V is clearly 3S (above 2S overlap, below 3S/4S overlap)
    TEST_ASSERT_EQUAL_UINT8(3, lvcAutoDetectCells(11.1f));
}

int main() {
    UNITY_BEGIN();

    // Creep
    RUN_TEST(test_creep_enters_below_threshold);
    RUN_TEST(test_creep_stays_with_hysteresis);
    RUN_TEST(test_creep_exits_above_hysteresis);
    RUN_TEST(test_creep_force_switch);
    RUN_TEST(test_creep_output_forward);
    RUN_TEST(test_creep_output_turn);
    RUN_TEST(test_creep_output_3motor_third_passive);
    RUN_TEST(test_creep_output_clamped);

    // Resync
    RUN_TEST(test_resync_init);
    RUN_TEST(test_resync_hold_accumulates);
    RUN_TEST(test_resync_release_returns_angle);
    RUN_TEST(test_resync_below_threshold_cancels);
    RUN_TEST(test_resync_stick_angle);
    RUN_TEST(test_resync_stick_magnitude);
    RUN_TEST(test_resync_wrap_around_backward);

    // Hit logger
    RUN_TEST(test_hitlog_init_empty);
    RUN_TEST(test_hitlog_record_and_retrieve);
    RUN_TEST(test_hitlog_max_g_tracked);
    RUN_TEST(test_hitlog_wraps);
    RUN_TEST(test_hitlog_format);

    // LVC
    RUN_TEST(test_lvc_auto_detect_3s);
    RUN_TEST(test_lvc_auto_detect_4s);
    RUN_TEST(test_lvc_auto_detect_too_low);
    RUN_TEST(test_lvc_update_sensor_fault);
    RUN_TEST(test_lvc_update_cell_count_unknown);
    RUN_TEST(test_lvc_update_ok);
    RUN_TEST(test_lvc_update_warn);
    RUN_TEST(test_lvc_update_critical);
    RUN_TEST(test_lvc_spindown_ramp);
    RUN_TEST(test_lvc_auto_detect_sticks);

    // Blackbox
    RUN_TEST(test_blackbox_init);
    RUN_TEST(test_blackbox_capacity);
    RUN_TEST(test_blackbox_next_offset);
    RUN_TEST(test_blackbox_wraps);
    RUN_TEST(test_blackbox_stored);
    RUN_TEST(test_blackbox_read_offset);
    RUN_TEST(test_blackbox_format);
    RUN_TEST(test_blackbox_format_header);
    RUN_TEST(test_blackbox_defers_on_busy);

    // Phase G: Cell ambiguity
    RUN_TEST(test_cell_detect_12_4v_ambiguous);
    RUN_TEST(test_cell_detect_9_0v_ambiguous);
    RUN_TEST(test_cell_detect_11_1v_unambiguous_3s);
    return UNITY_END();
}

