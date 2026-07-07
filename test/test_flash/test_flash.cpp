// MeltyFC — A/B Config Flash + Fault Handler Tests

#include "config_flash.hpp"
#include "config_store.hpp"
#include "fault_handler.hpp"
#include <cstring>
#include <unity.h>

using namespace melty;

// ============================================================================
// A/B Slot Logic
// ============================================================================

void test_validate_slot_valid() {
    ConfigData cfg;
    cfg.crc32 = computeConfigCrc(cfg);
    ConfigSlotHeader hdr = buildSlotHeader(5);

    auto status = validateSlot(hdr, cfg);
    TEST_ASSERT_TRUE(status.valid);
    TEST_ASSERT_EQUAL_UINT8(5, status.sequence);
}

void test_validate_slot_bad_magic() {
    ConfigData cfg;
    cfg.crc32 = computeConfigCrc(cfg);
    ConfigSlotHeader hdr = buildSlotHeader(5);
    hdr.magic = 0x12345678; // Wrong

    auto status = validateSlot(hdr, cfg);
    TEST_ASSERT_FALSE(status.valid);
}

void test_validate_slot_bad_crc() {
    ConfigData cfg;
    cfg.crc32 = 0xDEADBEEF; // Wrong CRC
    ConfigSlotHeader hdr = buildSlotHeader(5);

    auto status = validateSlot(hdr, cfg);
    TEST_ASSERT_FALSE(status.valid);
}

void test_pick_active_both_invalid() {
    SlotStatus s0 = {false, 0};
    SlotStatus s1 = {false, 0};
    TEST_ASSERT_EQUAL(-1, pickActiveSlot(s0, s1));
}

void test_pick_active_only_slot0() {
    SlotStatus s0 = {true, 3};
    SlotStatus s1 = {false, 0};
    TEST_ASSERT_EQUAL(0, pickActiveSlot(s0, s1));
}

void test_pick_active_only_slot1() {
    SlotStatus s0 = {false, 0};
    SlotStatus s1 = {true, 7};
    TEST_ASSERT_EQUAL(1, pickActiveSlot(s0, s1));
}

void test_pick_active_higher_sequence() {
    SlotStatus s0 = {true, 5};
    SlotStatus s1 = {true, 3};
    TEST_ASSERT_EQUAL(0, pickActiveSlot(s0, s1)); // 5 > 3
}

void test_pick_active_sequence_wraparound() {
    SlotStatus s0 = {true, 1};   // Just wrapped from 255
    SlotStatus s1 = {true, 254}; // Old
    // diff = 1 - 254 = 3 (unsigned wrap), 3 < 128 → slot 0 is newer
    TEST_ASSERT_EQUAL(0, pickActiveSlot(s0, s1));
}

void test_pick_write_overwrites_older() {
    SlotStatus s0 = {true, 5};
    SlotStatus s1 = {true, 3};
    // Active = 0 (higher seq), write to 1 (older)
    TEST_ASSERT_EQUAL(1, pickWriteSlot(s0, s1));
}

void test_pick_write_overwrites_invalid() {
    SlotStatus s0 = {true, 5};
    SlotStatus s1 = {false, 0};
    TEST_ASSERT_EQUAL(1, pickWriteSlot(s0, s1));
}

void test_next_sequence_wraps() {
    TEST_ASSERT_EQUAL_UINT8(0, nextSequence(255));
    TEST_ASSERT_EQUAL_UINT8(1, nextSequence(0));
    TEST_ASSERT_EQUAL_UINT8(128, nextSequence(127));
}

void test_slot_header_magic() {
    auto hdr = buildSlotHeader(42);
    TEST_ASSERT_EQUAL_UINT32(CONFIG_SLOT_MAGIC, hdr.magic);
    TEST_ASSERT_EQUAL_UINT8(42, hdr.sequence);
}

// ============================================================================
// Fault Handler
// ============================================================================

void test_fault_breadcrumb_valid() {
    FaultBreadcrumb fb = {};
    fb.magic = FAULT_MAGIC;
    TEST_ASSERT_TRUE(faultBreadcrumbValid(fb));
}

void test_fault_breadcrumb_invalid() {
    FaultBreadcrumb fb = {};
    fb.magic = 0;
    TEST_ASSERT_FALSE(faultBreadcrumbValid(fb));
}

void test_fault_breadcrumb_clear() {
    FaultBreadcrumb fb = {};
    fb.magic = FAULT_MAGIC;
    faultBreadcrumbClear(fb);
    TEST_ASSERT_FALSE(faultBreadcrumbValid(fb));
}

void test_fault_dump_format() {
    FaultBreadcrumb fb = {};
    fb.magic = FAULT_MAGIC;
    fb.pc = 0x08001234;
    fb.lr = 0x08005678;
    fb.cfsr = (1 << 25); // DIVBYZERO
    fb.faultType = 2;    // UsageFault
    fb.uptime_ms = 12345;

    char buf[512];
    int len = formatFaultDump(fb, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buf, "UsageFault"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "08001234"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "DIVBYZERO"));
}

void test_fault_dump_no_fault() {
    FaultBreadcrumb fb = {};
    fb.magic = 0; // Invalid

    char buf[128];
    formatFaultDump(fb, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "No fault"));
}

void test_cfsr_decode_multiple() {
    char buf[256];
    uint32_t cfsr = (1 << 16) | (1 << 25); // UNDEFINSTR + DIVBYZERO
    decodeCfsr(cfsr, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "UNDEFINSTR"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "DIVBYZERO"));
}

// ============================================================================
// Runner
// ============================================================================

int main() {
    UNITY_BEGIN();

    // A/B Slots
    RUN_TEST(test_validate_slot_valid);
    RUN_TEST(test_validate_slot_bad_magic);
    RUN_TEST(test_validate_slot_bad_crc);
    RUN_TEST(test_pick_active_both_invalid);
    RUN_TEST(test_pick_active_only_slot0);
    RUN_TEST(test_pick_active_only_slot1);
    RUN_TEST(test_pick_active_higher_sequence);
    RUN_TEST(test_pick_active_sequence_wraparound);
    RUN_TEST(test_pick_write_overwrites_older);
    RUN_TEST(test_pick_write_overwrites_invalid);
    RUN_TEST(test_next_sequence_wraps);
    RUN_TEST(test_slot_header_magic);

    // Fault Handler
    RUN_TEST(test_fault_breadcrumb_valid);
    RUN_TEST(test_fault_breadcrumb_invalid);
    RUN_TEST(test_fault_breadcrumb_clear);
    RUN_TEST(test_fault_dump_format);
    RUN_TEST(test_fault_dump_no_fault);
    RUN_TEST(test_cfsr_decode_multiple);

    return UNITY_END();
}
