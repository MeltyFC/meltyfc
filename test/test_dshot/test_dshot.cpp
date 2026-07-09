// MeltyFC — DShot Protocol Unit Tests
// Frame packing, CRC, GCR decode, timing calculations.

#include "dshot_common.hpp"
#include <unity.h>

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
    uint16_t frame = packFrame(5000, false); // Over 2047
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(2047, throttle);
}

void test_crc_validates_correct() {
    uint16_t frame = packFrame(500, true);
    TEST_ASSERT_TRUE(validateCrc(frame));
}

void test_crc_rejects_corrupt() {
    uint16_t frame = packFrame(500, true);
    frame ^= 0x0001; // Flip a CRC bit
    TEST_ASSERT_FALSE(validateCrc(frame));
}

void test_crc_rejects_data_corruption() {
    uint16_t frame = packFrame(500, true);
    frame ^= 0x0020; // Flip a data bit
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

// Finding 2: All DShot modes at 84 MHz
void test_timing_dshot150_84mhz() {
    auto timing = calculateTiming(84000000, 150000);
    TEST_ASSERT_EQUAL_UINT16(560, timing.bitPeriodTicks);
}

void test_timing_dshot600_84mhz() {
    auto timing = calculateTiming(84000000, 600000);
    TEST_ASSERT_EQUAL_UINT16(140, timing.bitPeriodTicks);
}

void test_timing_dshot1200_84mhz() {
    auto timing = calculateTiming(84000000, 1200000);
    TEST_ASSERT_EQUAL_UINT16(70, timing.bitPeriodTicks);
}

// Finding 2: Reject bare mode numbers (300 instead of 300000)
void test_timing_rejects_bare_mode_number() {
    auto timing = calculateTiming(84000000, 300);       // Wrong! Should be 300000
    TEST_ASSERT_EQUAL_UINT16(0, timing.bitPeriodTicks); // Returns zero = error
}

void test_timing_rejects_zero() {
    auto timing = calculateTiming(84000000, 0);
    TEST_ASSERT_EQUAL_UINT16(0, timing.bitPeriodTicks);
}

// Finding 6: Command frame preserves command ID
void test_command_frame_preserves_id() {
    uint16_t frame = packCommandFrame(13, true); // EDT enable
    uint16_t command = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(13, command); // Must be 13, not clamped to 48
}

void test_throttle_frame_clamps_to_48() {
    uint16_t frame = packThrottleFrame(13, false);
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(48, throttle); // Clamped from 13 to 48
}

void test_command_bidir_preserves_id() {
    uint16_t frame = packCommandBidir(13, true);
    uint16_t command = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(13, command);
}

void test_compare_buffer_encoding() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[DSHOT_COMPARE_BUF_SIZE];
    uint16_t frame = 0xAAAA; // Alternating bits

    encodeToCompareBuffer(frame, timing, buf);

    // MSB first: bit 15 = 1, bit 14 = 0, bit 13 = 1, ...
    TEST_ASSERT_EQUAL_UINT16(timing.bit1HighTicks, buf[0]); // bit 15 = 1
    TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[1]); // bit 14 = 0
    TEST_ASSERT_EQUAL_UINT16(timing.bit1HighTicks, buf[2]); // bit 13 = 1
    TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[3]); // bit 12 = 0
}

void test_compare_buffer_all_zeros() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[DSHOT_COMPARE_BUF_SIZE];
    encodeToCompareBuffer(0x0000, timing, buf);

    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT16(timing.bit0HighTicks, buf[i]);
    }
}

void test_compare_buffer_all_ones() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[DSHOT_COMPARE_BUF_SIZE];
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
    // Frame: [start:1][nibble3:5][nibble2:5][nibble1:5][nibble0:5] — F-03: start bit required
    uint32_t gcr = (1u << 20) | (25 << 0) | (27 << 5) | (18 << 10) | (19 << 15);
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
    // EDT encoding: 3-bit exponent + 9-bit mantissa in upper 12 bits
    // exponent=0, mantissa=200 → period = 200 << 0 = 200µs
    uint16_t raw12 = (0 << 9) | 200;
    uint16_t frame = (raw12 << 4) | 0x05; // CRC=5 (dummy)
    TEST_ASSERT_EQUAL_UINT16(200, extractErpmPeriod(frame));
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
    // Construct a frame with valid INVERTED CRC (bidir telemetry)
    uint16_t period = 1234;
    uint8_t crc = (~(period ^ (period >> 4) ^ (period >> 8))) & 0x0F;
    uint16_t frame = (period << 4) | crc;
    TEST_ASSERT_TRUE(validateTelemetryCrc(frame));
}

void test_telemetry_crc_invalid() {
    uint16_t period = 1234;
    uint8_t crc = (~(period ^ (period >> 4) ^ (period >> 8))) & 0x0F;
    uint16_t frame = (period << 4) | (crc ^ 0x01); // Corrupt inverted CRC
    TEST_ASSERT_FALSE(validateTelemetryCrc(frame));
}

// ============================================================================
// DShot commands
// ============================================================================

void test_beep_command_packs() {
    // Commands (1-47) are clamped to DSHOT_MIN_THROTTLE (48) by packFrame
    // to prevent accidental ESC reconfiguration. Direct command sending
    // uses a different path (not packFrame).
    uint16_t frame = packFrame(DSHOT_CMD_BEEP1, false);
    TEST_ASSERT_TRUE(validateCrc(frame));
    uint16_t throttle = (frame >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(DSHOT_MIN_THROTTLE, throttle); // Clamped
}

void test_throttle_never_1_to_47() {
    // Values 1-47 are ESC commands — packFrame MUST clamp to 48
    for (uint16_t t = 1; t < DSHOT_MIN_THROTTLE; t++) {
        uint16_t frame = packFrame(t, false);
        uint16_t throttle = (frame >> 5) & 0x7FF;
        TEST_ASSERT_EQUAL_UINT16(DSHOT_MIN_THROTTLE, throttle);
    }
    // 0 stays 0 (disarm)
    uint16_t frame0 = packFrame(0, false);
    uint16_t t0 = (frame0 >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(0, t0);
    // 48 stays 48
    uint16_t frame48 = packFrame(48, false);
    uint16_t t48 = (frame48 >> 5) & 0x7FF;
    TEST_ASSERT_EQUAL_UINT16(48, t48);
}

void test_bidir_crc_inverted() {
    uint16_t normal = packFrame(1000, true);
    uint16_t bidir = packFrameBidir(1000, true);
    // Same throttle + telem, but CRC differs (inverted)
    uint8_t normalCrc = normal & 0x0F;
    uint8_t bidirCrc = bidir & 0x0F;
    TEST_ASSERT_EQUAL_HEX8((~normalCrc) & 0x0F, bidirCrc);
}

void test_erpm_mantissa_exponent() {
    // EDT encoding: 3-bit exponent + 9-bit mantissa
    // period_us = mantissa << exponent
    // Example: exponent=2, mantissa=100 → period = 100 << 2 = 400
    uint16_t raw12 = (2 << 9) | 100;   // exp=2, mantissa=100
    uint16_t frame = (raw12 << 4) | 0; // Fake CRC=0
    uint16_t period = extractErpmPeriod(frame);
    TEST_ASSERT_EQUAL_UINT16(400, period);

    // exponent=0 → period = mantissa directly
    raw12 = (0 << 9) | 250;
    frame = (raw12 << 4) | 0;
    period = extractErpmPeriod(frame);
    TEST_ASSERT_EQUAL_UINT16(250, period);

    // exponent=7, mantissa=1 → period = 128
    raw12 = (7 << 9) | 1;
    frame = (raw12 << 4) | 0;
    period = extractErpmPeriod(frame);
    TEST_ASSERT_EQUAL_UINT16(128, period);
}

void test_telemetry_crc_inverted() {
    // Build a frame with known period and inverted CRC
    uint16_t period = 0x123;
    uint8_t crc = (~(period ^ (period >> 4) ^ (period >> 8))) & 0x0F;
    uint16_t frame = (period << 4) | crc;
    TEST_ASSERT_TRUE(validateTelemetryCrc(frame));

    // Non-inverted CRC should fail
    uint8_t wrongCrc = (period ^ (period >> 4) ^ (period >> 8)) & 0x0F;
    uint16_t badFrame = (period << 4) | wrongCrc;
    TEST_ASSERT_FALSE(validateTelemetryCrc(badFrame));
}

void test_compare_buffer_17th_slot() {
    auto timing = calculateTiming(84000000, 300000);
    uint16_t buf[DSHOT_COMPARE_BUF_SIZE];
    encodeToCompareBuffer(0xFFFF, timing, buf);
    // 17th slot must be 0 (trailing idle)
    TEST_ASSERT_EQUAL_UINT16(0, buf[16]);
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

    // Finding 6: Command vs throttle frames
    RUN_TEST(test_command_frame_preserves_id);
    RUN_TEST(test_throttle_frame_clamps_to_48);
    RUN_TEST(test_command_bidir_preserves_id);

    // Timer encoding — all DShot modes
    RUN_TEST(test_timing_dshot150_84mhz);
    RUN_TEST(test_timing_dshot300_84mhz);
    RUN_TEST(test_timing_dshot600_84mhz);
    RUN_TEST(test_timing_dshot1200_84mhz);
    RUN_TEST(test_timing_rejects_bare_mode_number);
    RUN_TEST(test_timing_rejects_zero);
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

    // Commands + safety
    RUN_TEST(test_beep_command_packs);
    RUN_TEST(test_throttle_never_1_to_47);

    // Bidirectional DShot
    RUN_TEST(test_bidir_crc_inverted);
    RUN_TEST(test_erpm_mantissa_exponent);
    RUN_TEST(test_telemetry_crc_inverted);
    RUN_TEST(test_compare_buffer_17th_slot);

    return UNITY_END();
}
