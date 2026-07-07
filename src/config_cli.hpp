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
    const char* arg1;       // For GET: param name. For SET: param name.
    const char* arg2;       // For SET: value string.
};

// Parse a CLI input line into a command + args.
// Input is modified in-place (null terminators inserted).
// Returns parsed command struct.
CliParsed parseCli(char* input);

// ============================================================================
// CLI output formatters (write to buffer, return bytes written)
// ============================================================================

// Format `get <name>` response: "NAME = VALUE UNIT\n"
int formatGet(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen);

// Format `dump` output: all params, one per line "set NAME VALUE\n" (replayable)
int formatDump(const ConfigData& cfg, char* buf, size_t bufLen);

// Format `list` output: one param per line with type/range/description
int formatList(char* buf, size_t bufLen);

// Format `list json` output: JSON array of param definitions
int formatListJson(char* buf, size_t bufLen);

// Format `help` output
int formatHelp(char* buf, size_t bufLen);

// Format `version` output
int formatVersion(char* buf, size_t bufLen);

// Validate a set command and apply to config.
// Returns error message (nullptr on success).
const char* executeSet(ConfigData& cfg, const char* name, const char* valueStr);

} // namespace melty
