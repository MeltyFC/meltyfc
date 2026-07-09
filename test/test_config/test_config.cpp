// MeltyFC — Config Store + CLI Unit Tests

#include "config_cli.hpp"
#include "config_store.hpp"
#include "param_registry.h"
#include <cstdio>
#include <cstring>
#include <unity.h>

using namespace melty;

// ============================================================================
// Param Registry
// ============================================================================

void test_registry_not_empty() {
    TEST_ASSERT_GREATER_THAN(0, PARAM_REGISTRY_SIZE);
}

void test_registry_find_known_param() {
    const ParamDef* def = findParam("MAX_RPM");
    TEST_ASSERT_NOT_NULL(def);
    TEST_ASSERT_EQUAL_STRING("rpm", def->unit);
    TEST_ASSERT_EQUAL_FLOAT(3200.0f, def->defaultVal);
}

void test_registry_find_unknown_returns_null() {
    TEST_ASSERT_NULL(findParam("NONEXISTENT_PARAM"));
}

void test_registry_all_params_have_names() {
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE; i++) {
        TEST_ASSERT_NOT_NULL(PARAM_REGISTRY[i].name);
        TEST_ASSERT_GREATER_THAN(0, strlen(PARAM_REGISTRY[i].name));
    }
}

void test_registry_no_duplicate_names() {
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE; i++) {
        for (size_t j = i + 1; j < PARAM_REGISTRY_SIZE; j++) {
            TEST_ASSERT_NOT_EQUAL_MESSAGE(0, strcmp(PARAM_REGISTRY[i].name, PARAM_REGISTRY[j].name),
                                          PARAM_REGISTRY[i].name);
        }
    }
}

// ============================================================================
// Get/Set
// ============================================================================

void test_get_default_value() {
    ConfigData cfg;
    const ParamDef* def = findParam("WINDOW_HALF");
    TEST_ASSERT_NOT_NULL(def);
    float val = getParamFloat(cfg, *def);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, val);
}

void test_set_value_clamped() {
    ConfigData cfg;
    const ParamDef* def = findParam("MAX_RPM");
    TEST_ASSERT_NOT_NULL(def);
    // Set above max
    setParamFloat(cfg, *def, 99999.0f);
    TEST_ASSERT_EQUAL_FLOAT(10000.0f, getParamFloat(cfg, *def));
    // Set below min
    setParamFloat(cfg, *def, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, getParamFloat(cfg, *def));
}

void test_set_float_precision() {
    ConfigData cfg;
    const ParamDef* def = findParam("THROTTLE_CAP");
    TEST_ASSERT_NOT_NULL(def);
    setParamFloat(cfg, *def, 0.85f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.85f, getParamFloat(cfg, *def));
}

void test_failsafe_floor() {
    ConfigData cfg;
    const ParamDef* def = findParam("FAILSAFE_MS");
    TEST_ASSERT_NOT_NULL(def);
    TEST_ASSERT(def->flags & ParamFlags::FLOOR);
    // Try to set below floor
    setParamFloat(cfg, *def, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, getParamFloat(cfg, *def));
}

// ============================================================================
// Format
// ============================================================================

void test_format_uint() {
    ConfigData cfg;
    const ParamDef* def = findParam("MAX_RPM");
    TEST_ASSERT_NOT_NULL(def);
    char buf[32];
    formatParam(cfg, *def, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("3200", buf);
}

void test_format_float() {
    ConfigData cfg;
    const ParamDef* def = findParam("THROTTLE_CAP");
    TEST_ASSERT_NOT_NULL(def);
    char buf[32];
    formatParam(cfg, *def, buf, sizeof(buf));
    // 0.9 should format as "0.9"
    TEST_ASSERT_EQUAL_STRING("0.9", buf);
}

// ============================================================================
// Config migration
// ============================================================================

void test_migrate_same_version() {
    ConfigData src;
    src.maxRpm = 2800;
    ConfigData dst;
    bool ok = migrateConfig(reinterpret_cast<const uint8_t*>(&src), sizeof(src), dst);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT16(2800, dst.maxRpm);
}

void test_migrate_bad_version() {
    uint8_t badData[sizeof(ConfigData)];
    memset(badData, 0xFF, sizeof(badData));
    // Schema version = 0xFFFF (wrong)
    ConfigData dst;
    bool ok = migrateConfig(badData, sizeof(badData), dst);
    TEST_ASSERT_FALSE(ok);
    // Should get defaults
    TEST_ASSERT_EQUAL_UINT16(3200, dst.maxRpm);
}

void test_migrate_null_data() {
    ConfigData dst;
    bool ok = migrateConfig(nullptr, 0, dst);
    TEST_ASSERT_FALSE(ok);
}

// ============================================================================
// CRC
// ============================================================================

void test_config_crc_deterministic() {
    ConfigData cfg;
    uint32_t crc1 = computeConfigCrc(cfg);
    uint32_t crc2 = computeConfigCrc(cfg);
    TEST_ASSERT_EQUAL_UINT32(crc1, crc2);
}

void test_config_crc_changes_on_modify() {
    ConfigData cfg1, cfg2;
    cfg2.maxRpm = 5000;
    TEST_ASSERT_NOT_EQUAL(computeConfigCrc(cfg1), computeConfigCrc(cfg2));
}

// ============================================================================
// CLI Parser
// ============================================================================

void test_cli_parse_get() {
    char input[] = "get MAX_RPM";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::GET, p.command);
    TEST_ASSERT_EQUAL_STRING("MAX_RPM", p.arg1);
}

void test_cli_parse_set() {
    char input[] = "set MAX_RPM 2800";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::SET, p.command);
    TEST_ASSERT_EQUAL_STRING("MAX_RPM", p.arg1);
    TEST_ASSERT_EQUAL_STRING("2800", p.arg2);
}

void test_cli_parse_dump() {
    char input[] = "dump";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::DUMP, p.command);
}

void test_cli_parse_list_json() {
    char input[] = "list json";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::LIST_JSON, p.command);
}

void test_cli_parse_list_plain() {
    char input[] = "list";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::LIST, p.command);
}

void test_cli_parse_help() {
    char input[] = "help";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::HELP, p.command);
}

void test_cli_parse_question_mark() {
    char input[] = "?";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::HELP, p.command);
}

void test_cli_parse_unknown() {
    char input[] = "frobnicate";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::UNKNOWN, p.command);
}

void test_cli_parse_empty() {
    char input[] = "";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::NONE, p.command);
}

void test_cli_parse_case_insensitive() {
    char input[] = "GET MAX_RPM";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::GET, p.command);
}

void test_cli_parse_trailing_newline() {
    char input[] = "version\r\n";
    CliParsed p = parseCli(input);
    TEST_ASSERT_EQUAL(CliCommand::VERSION, p.command);
}

// ============================================================================
// CLI Execute Set
// ============================================================================

void test_cli_execute_set_valid() {
    ConfigData cfg;
    const char* err = executeSet(cfg, "MAX_RPM", "2800");
    TEST_ASSERT_NULL(err);
    TEST_ASSERT_EQUAL_UINT16(2800, cfg.maxRpm);
}

void test_cli_execute_set_unknown_param() {
    ConfigData cfg;
    const char* err = executeSet(cfg, "FAKE_PARAM", "100");
    TEST_ASSERT_NOT_NULL(err);
}

void test_cli_execute_set_invalid_value() {
    ConfigData cfg;
    const char* err = executeSet(cfg, "MAX_RPM", "abc");
    TEST_ASSERT_NOT_NULL(err);
}

void test_cli_execute_set_missing_args() {
    ConfigData cfg;
    TEST_ASSERT_NOT_NULL(executeSet(cfg, nullptr, "100"));
    TEST_ASSERT_NOT_NULL(executeSet(cfg, "MAX_RPM", nullptr));
}

// ============================================================================
// CLI Format Dump (round-trip)
// ============================================================================

void test_cli_dump_format() {
    ConfigData cfg;
    char buf[8192];
    int n = formatDump(cfg, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    // Should contain replayable set commands
    TEST_ASSERT_NOT_NULL(strstr(buf, "set MAX_RPM 3200"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "set WINDOW_HALF 30"));
}

void test_cli_list_json_valid() {
    char buf[16384];
    int n = formatListJson(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    // Should be valid-ish JSON (starts with [, ends with ])
    TEST_ASSERT_EQUAL('[', buf[0]);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"MAX_RPM\""));
}

void test_cli_version_output() {
    char buf[256];
    int n = formatVersion(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "MeltyFC"));
}

void test_cli_help_output() {
    char buf[1024];
    int n = formatHelp(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "get"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "set"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "dump"));
}

// ============================================================================
// Format Get
// ============================================================================

void test_cli_format_get() {
    ConfigData cfg;
    const ParamDef* def = findParam("MAX_RPM");
    TEST_ASSERT_NOT_NULL(def);
    char buf[128];
    int n = formatGet(cfg, *def, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_NOT_NULL(strstr(buf, "MAX_RPM"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "3200"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "rpm"));
}

// ============================================================================
// Main
// ============================================================================
// ============================================================================
// B5: CLI line buffer bounding
// ============================================================================

void test_line_buffer_basic() {
    CliLineBuffer lb;
    lb.init();
    const char* input = "get WINDOW_HALF\n";
    bool ready = false;
    for (int i = 0; input[i]; i++) {
        ready = lb.feed(input[i]);
    }
    TEST_ASSERT_TRUE(ready);
    TEST_ASSERT_EQUAL_STRING("get WINDOW_HALF", lb.line());
}

void test_line_buffer_overflow_discards() {
    CliLineBuffer lb;
    lb.init();
    // Feed 200 characters (exceeds CLI_MAX_LINE=128), then newline
    for (int i = 0; i < 200; i++) {
        lb.feed('x');
    }
    bool ready = lb.feed('\n');
    TEST_ASSERT_FALSE(ready); // Overflowed line is discarded
    // Buffer is reset for next line
    lb.feed('a');
    ready = lb.feed('\n');
    TEST_ASSERT_TRUE(ready);
    TEST_ASSERT_EQUAL_STRING("a", lb.line());
}

// CO-1: Default config MUST pass its own validator.
// One careless default edit + no validation at load = every fresh board boots
// the exact control-reversal config validateConfig was built to reject.
// This test pins default-set coherence forever.
void test_defaults_pass_validation() {
    ConfigData defaults = {};
    ConfigValidationResult result = validateConfig(defaults);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, result.issueCount,
        "Default config fails its own validator — fresh boards would boot invalid");
    TEST_ASSERT_FALSE(result.spinMaxExceedsCap);
    TEST_ASSERT_FALSE(result.innerGtOuter);
    TEST_ASSERT_FALSE(result.lvcWarnBelowCrit);
    TEST_ASSERT_FALSE(result.windowHalfTooLarge);
    TEST_ASSERT_FALSE(result.channelCollision);
    TEST_ASSERT_FALSE(result.numMotorsInvalid);
}

int main() {
    UNITY_BEGIN();

    // Registry
    RUN_TEST(test_registry_not_empty);
    RUN_TEST(test_registry_find_known_param);
    RUN_TEST(test_registry_find_unknown_returns_null);
    RUN_TEST(test_registry_all_params_have_names);
    RUN_TEST(test_registry_no_duplicate_names);

    // Get/Set
    RUN_TEST(test_get_default_value);
    RUN_TEST(test_set_value_clamped);
    RUN_TEST(test_set_float_precision);
    RUN_TEST(test_failsafe_floor);

    // Format
    RUN_TEST(test_format_uint);
    RUN_TEST(test_format_float);

    // Migration
    RUN_TEST(test_migrate_same_version);
    RUN_TEST(test_migrate_bad_version);
    RUN_TEST(test_migrate_null_data);

    // CRC
    RUN_TEST(test_config_crc_deterministic);
    RUN_TEST(test_config_crc_changes_on_modify);

    // CLI Parser
    RUN_TEST(test_cli_parse_get);
    RUN_TEST(test_cli_parse_set);
    RUN_TEST(test_cli_parse_dump);
    RUN_TEST(test_cli_parse_list_json);
    RUN_TEST(test_cli_parse_list_plain);
    RUN_TEST(test_cli_parse_help);
    RUN_TEST(test_cli_parse_question_mark);
    RUN_TEST(test_cli_parse_unknown);
    RUN_TEST(test_cli_parse_empty);
    RUN_TEST(test_cli_parse_case_insensitive);
    RUN_TEST(test_cli_parse_trailing_newline);

    // CLI Execute
    RUN_TEST(test_cli_execute_set_valid);
    RUN_TEST(test_cli_execute_set_unknown_param);
    RUN_TEST(test_cli_execute_set_invalid_value);
    RUN_TEST(test_cli_execute_set_missing_args);

    // CLI Format
    RUN_TEST(test_cli_dump_format);
    RUN_TEST(test_cli_list_json_valid);
    RUN_TEST(test_cli_version_output);
    RUN_TEST(test_cli_help_output);
    RUN_TEST(test_cli_format_get);

    // B5: Line buffer
    RUN_TEST(test_line_buffer_basic);
    RUN_TEST(test_line_buffer_overflow_discards);

    // CO-1: Defaults coherence — pins this forever
    RUN_TEST(test_defaults_pass_validation);

    return UNITY_END();
}
