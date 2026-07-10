// MeltyFC — USB CDC CLI
// get/set/dump/save/defaults/status/cal/version/help/list
// Configurator substrate — future GUI autogenerates from `list json`.
// See spec §11B.

#pragma once

#include "param_registry.h"
#include <cstddef>

namespace melty {

// ============================================================================
// CLI command types
// ============================================================================
enum class CliCommand {
    NONE,
    GET,
    SET,
    DUMP,
    SAVE,
    DEFAULTS,
    LIST,
    LIST_JSON,
    STATUS,
    CAL,
    VERSION,
    HELP,
    UNKNOWN,
};

// ============================================================================
// Parsed command result
// ============================================================================
struct CliParsed {
    CliCommand command;
    const char* arg1; // For GET: param name. For SET: param name.
    const char* arg2; // For SET: value string.
};

// Parse a CLI input line into a command + args.
// Input is modified in-place (null terminators inserted).
// Returns parsed command struct.
[[nodiscard]] CliParsed parseCli(char* input);

// ============================================================================
// CLI output formatters (write to buffer, return bytes written)
// ============================================================================

// Format `get <name>` response: "NAME = VALUE UNIT\n"
[[nodiscard]] int formatGet(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen);

// Format `dump` output: all params, one per line "set NAME VALUE\n" (replayable)
[[nodiscard]] int formatDump(const ConfigData& cfg, char* buf, size_t bufLen);

// Format `list` output: one param per line with type/range/description
[[nodiscard]] int formatList(char* buf, size_t bufLen);

// Format `list json` output: JSON array of param definitions
[[nodiscard]] int formatListJson(char* buf, size_t bufLen);

// Format `help` output
[[nodiscard]] int formatHelp(char* buf, size_t bufLen);

// Format `version` output
[[nodiscard]] int formatVersion(char* buf, size_t bufLen);

// Validate a set command and apply to config.
// Returns error message (nullptr on success).
const char* executeSet(ConfigData& cfg, const char* name, const char* valueStr);

// ============================================================================
// B5: Bounded line accumulator for serial input
// Collects characters until newline, then provides the complete line.
// Overflow: discards excess characters until newline (never writes past buffer).
// ============================================================================
constexpr size_t CLI_MAX_LINE = 128;

struct CliLineBuffer {
    char buf[CLI_MAX_LINE];
    uint8_t pos;
    bool overflow;

    void init() {
        pos = 0;
        overflow = false;
        buf[0] = '\0';
    }

    // Feed one character. Returns true when a complete line is ready.
    [[nodiscard]] bool feed(char c) {
        if (c == '\n' || c == '\r') {
            if (pos == 0 && !overflow)
                return false; // Ignore empty lines
            buf[pos] = '\0';
            bool wasOverflow = overflow;
            overflow = false;
            if (wasOverflow) {
                pos = 0;
                return false; // Discard overflowed line
            }
            return true; // Line ready in buf
        }
        if (pos < CLI_MAX_LINE - 1) {
            buf[pos++] = c;
        } else {
            overflow = true; // Mark overflow, keep eating until newline
        }
        return false;
    }

    // Get the completed line (valid after feed returns true)
    char* line() { return buf; }

    // Reset after consuming the line
    void reset() {
        pos = 0;
        overflow = false;
    }
};

} // namespace melty
