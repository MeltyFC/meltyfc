// MeltyFC — CRSF Protocol Unit Tests

#include "crsf.hpp"
#include <cmath>
#include <cstring>
#include <unity.h>

using namespace melty;

// ============================================================================
// CRC8 DVB-S2
// ============================================================================

void test_crc8_zero() {
    TEST_ASSERT_EQUAL_UINT8(0, crc8DvbS2Block(nullptr, 0));
}

void test_crc8_known_vector() {
    // Known test: CRC8/DVB-S2 of "123456789" = 0xBC
    uint8_t data[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    TEST_ASSERT_EQUAL_HEX8(0xBC, crc8DvbS2Block(data, 9));
}

// ============================================================================
// Parser state machine
// ============================================================================

void test_parser_init() {
    CrsfParser parser;
    crsfParserInit(parser);
    TEST_ASSERT_EQUAL(CrsfParserState::SYNC, parser.state);
}

void test_parser_rejects_bad_sync() {
    CrsfParser parser;
    crsfParserInit(parser);
    TEST_ASSERT_FALSE(crsfParserFeed(parser, 0x00));
    TEST_ASSERT_FALSE(crsfParserFeed(parser, 0xFF));
    TEST_ASSERT_EQUAL(CrsfParserState::SYNC, parser.state);
}

void test_parser_accepts_sync() {
    CrsfParser parser;
    crsfParserInit(parser);
    (void)crsfParserFeed(parser, CRSF_SYNC_BYTE);
    TEST_ASSERT_EQUAL(CrsfParserState::LENGTH, parser.state);
}

void test_parser_rejects_bad_length() {
    CrsfParser parser;
    crsfParserInit(parser);
    (void)crsfParserFeed(parser, CRSF_SYNC_BYTE);
    (void)crsfParserFeed(parser, 0); // Length 0 = invalid
    TEST_ASSERT_EQUAL(CrsfParserState::SYNC, parser.state);
}

void test_parser_rejects_oversized_length() {
    CrsfParser parser;
    crsfParserInit(parser);
    (void)crsfParserFeed(parser, CRSF_SYNC_BYTE);
    (void)crsfParserFeed(parser, 63); // Too large
    TEST_ASSERT_EQUAL(CrsfParserState::SYNC, parser.state);
}

// Build a valid minimal frame and feed it
void test_parser_complete_frame() {
    // Build a simple frame: sync + length + type + crc
    // Type = 0x21 (flight mode), payload = "OK\0"
    uint8_t payload[] = {0x21, 'O', 'K', 0};
    uint8_t crc = crc8DvbS2Block(payload, 4);

    CrsfParser parser;
    crsfParserInit(parser);

    (void)crsfParserFeed(parser, CRSF_SYNC_BYTE); // sync
    (void)crsfParserFeed(parser, 5);              // length = type(1) + payload(3) + crc(1) = 5
    (void)crsfParserFeed(parser, 0x21);           // type
    (void)crsfParserFeed(parser, 'O');
    (void)crsfParserFeed(parser, 'K');
    (void)crsfParserFeed(parser, 0);
    bool result = crsfParserFeed(parser, crc); // CRC

    TEST_ASSERT_TRUE(result);
}

void test_parser_bad_crc_rejected() {
    CrsfParser parser;
    crsfParserInit(parser);

    (void)crsfParserFeed(parser, CRSF_SYNC_BYTE);
    (void)crsfParserFeed(parser, 3);    // length
    (void)crsfParserFeed(parser, 0x21); // type
    (void)crsfParserFeed(parser, 'X');
    bool result = crsfParserFeed(parser, 0xFF); // Wrong CRC

    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Channel conversions
// ============================================================================

void test_channel_to_float_center() {
    float val = crsfChannelToFloat(CRSF_CHANNEL_MID);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, val);
}

void test_channel_to_float_min() {
    float val = crsfChannelToFloat(CRSF_CHANNEL_MIN);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -1.0f, val);
}

void test_channel_to_float_max() {
    float val = crsfChannelToFloat(CRSF_CHANNEL_MAX);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, val);
}

void test_channel_to_throttle_min() {
    float val = crsfChannelToThrottle(CRSF_CHANNEL_MIN);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, val);
}

void test_channel_to_throttle_max() {
    float val = crsfChannelToThrottle(CRSF_CHANNEL_MAX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, val);
}

void test_channel_to_bool() {
    TEST_ASSERT_FALSE(crsfChannelToBool(CRSF_CHANNEL_MIN));
    TEST_ASSERT_TRUE(crsfChannelToBool(CRSF_CHANNEL_MAX));
}

// ============================================================================
// Telemetry frame builders
// ============================================================================

void test_build_flight_mode() {
    uint8_t buf[32];
    size_t len = crsfBuildFlightMode(buf, sizeof(buf), "UPRT");
    TEST_ASSERT_GREATER_THAN(0, len);
    // Verify sync byte
    TEST_ASSERT_EQUAL_HEX8(CRSF_ADDRESS_FC, buf[0]);
    // Verify type
    TEST_ASSERT_EQUAL_HEX8(CRSF_FRAMETYPE_FLIGHT_MODE, buf[2]);
    // Verify CRC (last byte)
    uint8_t crc = crc8DvbS2Block(&buf[2], len - 3);
    TEST_ASSERT_EQUAL_HEX8(crc, buf[len - 1]);
}

void test_build_flight_mode_truncates_long_text() {
    uint8_t buf[32];
    size_t len = crsfBuildFlightMode(buf, sizeof(buf), "THIS IS A VERY LONG MODE STRING");
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_LESS_THAN(25, len); // Should be truncated
}

void test_build_battery() {
    uint8_t buf[16];
    size_t len = crsfBuildBattery(buf, sizeof(buf), 11.1f, 5.0f, 500, 75);
    TEST_ASSERT_EQUAL(12, len);
    TEST_ASSERT_EQUAL_HEX8(CRSF_FRAMETYPE_BATTERY, buf[2]);
    // Verify voltage (11.1V × 10 = 111 = 0x006F big-endian)
    TEST_ASSERT_EQUAL_HEX8(0x00, buf[3]);
    TEST_ASSERT_EQUAL_HEX8(0x6F, buf[4]);
}

void test_build_buffer_too_small() {
    uint8_t buf[2];
    TEST_ASSERT_EQUAL(0, crsfBuildFlightMode(buf, sizeof(buf), "UPRT"));
    TEST_ASSERT_EQUAL(0, crsfBuildBattery(buf, sizeof(buf), 11.1f, 5.0f, 0, 0));
}

// Phase D: F-05 clamp edge tests
void test_channel_to_float_clamp_low() {
    float val = crsfChannelToFloat(0);
    TEST_ASSERT_TRUE(val >= -1.0f);
}
void test_channel_to_float_clamp_high() {
    float val = crsfChannelToFloat(2047);
    TEST_ASSERT_TRUE(val <= 1.0f);
}
void test_channel_to_throttle_clamp_zero() {
    float val = crsfChannelToThrottle(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, val);
}
void test_channel_to_throttle_clamp_max() {
    float val = crsfChannelToThrottle(2047);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, val);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    UNITY_BEGIN();

    // CRC
    RUN_TEST(test_crc8_zero);
    RUN_TEST(test_crc8_known_vector);

    // Parser
    RUN_TEST(test_parser_init);
    RUN_TEST(test_parser_rejects_bad_sync);
    RUN_TEST(test_parser_accepts_sync);
    RUN_TEST(test_parser_rejects_bad_length);
    RUN_TEST(test_parser_rejects_oversized_length);
    RUN_TEST(test_parser_complete_frame);
    RUN_TEST(test_parser_bad_crc_rejected);

    // Channel conversions
    RUN_TEST(test_channel_to_float_center);
    RUN_TEST(test_channel_to_float_min);
    RUN_TEST(test_channel_to_float_max);
    RUN_TEST(test_channel_to_throttle_min);
    RUN_TEST(test_channel_to_throttle_max);
    RUN_TEST(test_channel_to_bool);

    // Telemetry
    RUN_TEST(test_build_flight_mode);
    RUN_TEST(test_build_flight_mode_truncates_long_text);
    RUN_TEST(test_build_battery);
    RUN_TEST(test_build_buffer_too_small);

    // Phase D: F-05 clamp edge cases
    RUN_TEST(test_channel_to_float_clamp_low);
    RUN_TEST(test_channel_to_float_clamp_high);
    RUN_TEST(test_channel_to_throttle_clamp_zero);
    RUN_TEST(test_channel_to_throttle_clamp_max);

    return UNITY_END();
}
