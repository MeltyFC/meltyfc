// MeltyFC — DShot Protocol Unit Tests
// Frame packing, CRC, GCR decode, timing calculations.

#include <unity.h>
#include "../src/dshot_common.hpp"

using namespace melty::dshot;

// ============================================================================
// Frame packing & CRC
// ============================================================================

void test_pack_disarm() {
    uint16_t frame = packFrame(0, false);
    // Throttle 0, telem 0 → payload = 0x000, CRC = 0^0^0 = 0
    TEST_ASSERT_EQUAL_HEX16(0x0000, frame);
    TEST_ASSERT_TRUE(validateCrc(frame));
}

void test_pack_min_throttle() {
    uint16_t frame = packFrame(48, false);
    TEST_ASSERT_TRUE(validateCrc(frame));
    // Verify throttle bits
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(48, throttle);
}

void test_pack_max_throttle() {
    uint16_t frame = packFrame(2047, false);
    TEST_ASSERT_TRUE(validateCrc(frame));
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(2047, throttle);
}

void test_pack_with_telemetry_bit() {
    uint16_t frame = packFrame(1000, true);
    TEST_ASSERT_TRUE(validateCrc(frame));
    // Telemetry bit is bit 4
    TEST_ASSERT_TRUE((frame >> 4) & 1);
}

void test_pack_without_telemetry_bit() {
    uint16_t frame = packFrame(1000, false);
    TEST_ASSERT_TRUE(validateCrc(frame));
    TEST_ASSERT_FALSE((frame >> 4) & 1);
}

void test_pack_clamps_throttle() {
    uint16_t frame = packFrame(5000, false);  // Over 2047
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(2047, throttle);
}

void test_crc_validates_correct() {
    uint16_t frame = packFrame(500, true);
    TEST_ASSERT_TRUE(validateCrc(frame));
}

void test_crc_rejects_corrupt() {
    uint16_t frame = packFrame(500, true);
    frame ^= 0x0001;  // Flip a CRC bit
    TEST_ASSERT_FALSE(validateCrc(frame));
}

void test_crc_rejects_data_corruption() {
    uint16_t frame = packFrame(500, true);
    frame ^= 0x0020;  // Flip a data bit
    TEST_ASSERT_FALSE(validateCrc(frame));
}

// ============================================================================
// Timer compare-buffer encoding
// ============================================================================

void test_timing_dshot300_84mhz() {
    // 84MHz timer, DShot300 = 300kbps
    auto timing = calculateTiming(84000000, 300000);
    // Bit period = 84M / 300k = 280 ticks
    TEST_ASSERT_EQUAL_UINT16(280, timing.bitPeriodTicks);
    // Bit 1 = 75% = 210
    TEST_ASSERT_EQUAL_UINT16(210, timing.bit1HighTicks);
    // Bit 0 = 37.5% = 105
    TEST_ASSERT_EQUAL_UINT16(105, timing.bit0HighTicks);
}

void test_compare_buffer_encoding() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[16];
    uint16_t frame = 0xAAAA;  // Alternating bits

    encodeToCompareBuffer(frame, timing, buf);

    // MSB first: bit 15 = 1, bit 14 = 0, bit 13 = 1, ...
    TEST_ASSERT_EQUAL_UINT16(timing.bit1HighTicks, buf[0]);   // bit 15 = 1
    TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[1]);   // bit 14 = 0
    TEST_ASSERT_EQUAL_UINT16(timing.bit1HighTicks, buf[2]);   // bit 13 = 1
    TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[3]);   // bit 12 = 0
}

void test_compare_buffer_all_zeros() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[16];
    encodeToCompareBuffer(0x0000, timing, buf);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[i]);
    }
}

void test_compare_buffer_all_ones() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[16];
    encodeToCompareBuffer(0xFFFF, timing, buf);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT16(timing.bit1HighTicks, buf[i]);
    }
}

// ============================================================================
// GCR decode
// ============================================================================

void test_gcr_decode_valid() {
    // Encode nibbles 0x0, 0x1, 0x2, 0x3 via GCR
    // GCR for 0x0 = 11001 (25), 0x1 = 11011 (27), 0x2 = 10010 (18), 0x3 = 10011 (19)
    // Frame: [nibble0=25][nibble1=27][nibble2=18][nibble3=19] = least significant first
    uint32_t gcr = (25 << 0) | (27 << 5) | (18 << 10) | (19 << 15);
    uint16_t decoded = gcrDecode(gcr);
    TEST_ASSERT_NOT_EQUAL_HEX16(0xFFFF, decoded);

    // Verify nibbles
    TEST_ASSERT_EQUAL_HEX8(0x0, decoded & 0xF);
    TEST_ASSERT_EQUAL_HEX8(0x1, (decoded >> 4) & 0xF);
    TEST_ASSERT_EQUAL_HEX8(0x2, (decoded >> 8) & 0xF);
    TEST_ASSERT_EQUAL_HEX8(0x3, (decoded >> 12) & 0xF);
}

void test_gcr_decode_invalid() {
    // 00000 is not a valid GCR encoding
    uint32_t gcr = 0;
    uint16_t decoded = gcrDecode(gcr);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, decoded);
}

void test_gcr_all_valid_entries() {
    // Verify every valid GCR code decodes to a valid nibble
    int validCount = 0;
    for (uint8_t i = 0; i < 32; i++) {
        if (GCR_DECODE_TABLE[i] != 0xFF) {
            TEST_ASSERT(GCR_DECODE_TABLE[i] <= 0x0F);
            validCount++;
        }
    }
    // Should have exactly 16 valid entries (0x0–0xF)
    TEST_ASSERT_EQUAL_INT(16, validCount);
}

void test_gcr_no_duplicate_mappings() {
    // Each nibble 0x0–0xF should appear exactly once
    uint8_t seen[16] = {0};
    for (uint8_t i = 0; i < 32; i++) {
        if (GCR_DECODE_TABLE[i] != 0xFF) {
            seen[GCR_DECODE_TABLE[i]]++;
        }
    }
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT8(1, seen[i]);
    }
}

// ============================================================================
// Telemetry frame
// ============================================================================

void test_erpm_period_extraction() {
    // Decoded frame: period in upper 12 bits, CRC in lower 4
    uint16_t frame = (1234 << 4) | 0x05;  // Period=1234, CRC=5
    TEST_ASSERT_EQUAL_UINT16(1234, extractErpmPeriod(frame));
}

void test_period_to_erpm() {
    // Period 100µs → eRPM = 60,000,000 / 100 = 600,000
    TEST_ASSERT_EQUAL_UINT32(600000, periodToErpm(100));
}

void test_period_to_erpm_zero() {
    TEST_ASSERT_EQUAL_UINT32(0, periodToErpm(0));
}

void test_period_to_erpm_realistic() {
    // 14-pole motor at 3000 mechanical RPM = 21000 eRPM
    // Period = 60,000,000 / 21000 ≈ 2857µs
    uint32_t erpm = periodToErpm(2857);
    TEST_ASSERT_UINT32_WITHIN(100, 21000, erpm);
}

void test_telemetry_crc_valid() {
    // Construct a frame with valid CRC
    uint16_t period = 1234;
    uint8_t crc = (period ^ (period >> 4) ^ (period >> 8)) & 0x0F;
    uint16_t frame = (period << 4) | crc;
    TEST_ASSERT_TRUE(validateTelemetryCrc(frame));
}

void test_telemetry_crc_invalid() {
    uint16_t period = 1234;
    uint8_t crc = (period ^ (period >> 4) ^ (period >> 8)) & 0x0F;
    uint16_t frame = (period << 4) | (crc ^ 0x01);  // Corrupt CRC
    TEST_ASSERT_FALSE(validateTelemetryCrc(frame));
}

// ============================================================================
// DShot commands
// ============================================================================

void test_beep_command_packs() {
    uint16_t frame = packFrame(DSHOT_CMD_BEEP1, false);
    TEST_ASSERT_TRUE(validateCrc(frame));
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(1, throttle);
}

// ============================================================================
// Runner
// ============================================================================

int main() {
    UNITY_BEGIN();

    // Frame packing
    RUN_TEST(test_pack_disarm);
    RUN_TEST(test_pack_min_throttle);
    RUN_TEST(test_pack_max_throttle);
    RUN_TEST(test_pack_with_telemetry_bit);
    RUN_TEST(test_pack_without_telemetry_bit);
    RUN_TEST(test_pack_clamps_throttle);

    // CRC
    RUN_TEST(test_crc_validates_correct);
    RUN_TEST(test_crc_rejects_corrupt);
    RUN_TEST(test_crc_rejects_data_corruption);

    // Timer encoding
    RUN_TEST(test_timing_dshot300_84mhz);
    RUN_TEST(test_compare_buffer_encoding);
    RUN_TEST(test_compare_buffer_all_zeros);
    RUN_TEST(test_compare_buffer_all_ones);

    // GCR decode
    RUN_TEST(test_gcr_decode_valid);
    RUN_TEST(test_gcr_decode_invalid);
    RUN_TEST(test_gcr_all_valid_entries);
    RUN_TEST(test_gcr_no_duplicate_mappings);

    // Telemetry
    RUN_TEST(test_erpm_period_extraction);
    RUN_TEST(test_period_to_erpm);
    RUN_TEST(test_period_to_erpm_zero);
    RUN_TEST(test_period_to_erpm_realistic);
    RUN_TEST(test_telemetry_crc_valid);
    RUN_TEST(test_telemetry_crc_invalid);

    // Commands
    RUN_TEST(test_beep_command_packs);

    return UNITY_END();
}
