// MeltyFC — USB CDC CLI Implementation
// See spec §11B.

#include "config_cli.hpp"
#include "param_registry.h"
#include "config_store.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace melty {

// ============================================================================
// Tokenizer — split input into command + up to 2 args
// ============================================================================
static void skipWhitespace(char*& p) {
    while (*p == ' ' || *p == '\t') p++;
}

static char* nextToken(char*& p) {
    skipWhitespace(p);
    if (*p == '\0') return nullptr;
    char* start = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    if (*p != '\0') { *p = '\0'; p++; }
    return start;
}

// ============================================================================
// Parse CLI input
// ============================================================================
CliParsed parseCli(char* input) {
    CliParsed result = {CliCommand::NONE, nullptr, nullptr};
    if (input == nullptr) return result;

    // Strip trailing newline/CR
    size_t len = strlen(input);
    while (len > 0 && (input[len-1] == '\n' || input[len-1] == '\r')) {
        input[--len] = '\0';
    }

    char* p = input;
    char* cmd = nextToken(p);
    if (cmd == nullptr) return result;

    // Case-insensitive command matching
    // Convert command to lowercase in-place
    for (char* c = cmd; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') *c += 32;
    }

    if (strcmp(cmd, "get") == 0) {
        result.command = CliCommand::GET;
        result.arg1 = nextToken(p);
    } else if (strcmp(cmd, "set") == 0) {
        result.command = CliCommand::SET;
        result.arg1 = nextToken(p);
        result.arg2 = nextToken(p);
    } else if (strcmp(cmd, "dump") == 0) {
        result.command = CliCommand::DUMP;
    } else if (strcmp(cmd, "save") == 0) {
        result.command = CliCommand::SAVE;
    } else if (strcmp(cmd, "defaults") == 0) {
        result.command = CliCommand::DEFAULTS;
    } else if (strcmp(cmd, "list") == 0) {
        char* arg = nextToken(p);
        if (arg && strcmp(arg, "json") == 0) {
            result.command = CliCommand::LIST_JSON;
        } else {
            result.command = CliCommand::LIST;
        }
    } else if (strcmp(cmd, "status") == 0) {
        result.command = CliCommand::STATUS;
    } else if (strcmp(cmd, "cal") == 0) {
        result.command = CliCommand::CAL;
        result.arg1 = nextToken(p);
    } else if (strcmp(cmd, "version") == 0) {
        result.command = CliCommand::VERSION;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        result.command = CliCommand::HELP;
    } else {
        result.command = CliCommand::UNKNOWN;
        result.arg1 = cmd;
    }

    return result;
}

// ============================================================================
// Format `get` response
// ============================================================================
int formatGet(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen) {
    char valBuf[32];
    formatParam(cfg, def, valBuf, sizeof(valBuf));

    if (def.unit[0] != '\0') {
        return snprintf(buf, bufLen, "%s = %s %s\r\n", def.name, valBuf, def.unit);
    }
    return snprintf(buf, bufLen, "%s = %s\r\n", def.name, valBuf);
}

// ============================================================================
// Format `dump` — Betaflight-style replayable output
// ============================================================================
int formatDump(const ConfigData& cfg, char* buf, size_t bufLen) {
    int written = 0;
    int n;

    n = snprintf(buf + written, bufLen - written,
        "# MeltyFC config dump — schema v%u\r\n", ConfigData::SCHEMA_VERSION);
    if (n > 0) written += n;

    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 64; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        char valBuf[32];
        formatParam(cfg, def, valBuf, sizeof(valBuf));
        n = snprintf(buf + written, bufLen - written, "set %s %s\r\n", def.name, valBuf);
        if (n > 0) written += n;
    }

    return written;
}

// ============================================================================
// Format `list` — human-readable param listing
// ============================================================================
int formatList(char* buf, size_t bufLen) {
    int written = 0;
    int n;

    n = snprintf(buf + written, bufLen - written,
        "%-24s %-8s %-8s %-8s %-6s %s\r\n",
        "NAME", "TYPE", "MIN", "MAX", "UNIT", "DESCRIPTION");
    if (n > 0) written += n;

    n = snprintf(buf + written, bufLen - written,
        "%-24s %-8s %-8s %-8s %-6s %s\r\n",
        "----", "----", "---", "---", "----", "-----------");
    if (n > 0) written += n;

    static const char* typeNames[] = {
        "uint8", "uint16", "uint32", "int8", "int16", "int32", "float", "bool", "enum"
    };

    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 128; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        const char* typeName = (uint8_t)def.type < 9 ? typeNames[(uint8_t)def.type] : "?";
        char minBuf[16], maxBuf[16];

        if (def.type == ParamType::FLOAT) {
            snprintf(minBuf, sizeof(minBuf), "%.2f", (double)def.min);
            snprintf(maxBuf, sizeof(maxBuf), "%.2f", (double)def.max);
        } else {
            snprintf(minBuf, sizeof(minBuf), "%d", (int)def.min);
            snprintf(maxBuf, sizeof(maxBuf), "%d", (int)def.max);
        }

        n = snprintf(buf + written, bufLen - written,
            "%-24s %-8s %-8s %-8s %-6s %s\r\n",
            def.name, typeName, minBuf, maxBuf, def.unit, def.description);
        if (n > 0) written += n;
    }

    return written;
}

// ============================================================================
// Format `list json` — machine-readable for configurator GUI
// ============================================================================
int formatListJson(char* buf, size_t bufLen) {
    int written = 0;
    int n;

    static const char* typeNames[] = {
        "uint8", "uint16", "uint32", "int8", "int16", "int32", "float", "bool", "enum"
    };

    n = snprintf(buf + written, bufLen - written, "[\r\n");
    if (n > 0) written += n;

    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 256; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        const char* typeName = (uint8_t)def.type < 9 ? typeNames[(uint8_t)def.type] : "unknown";
        const char* comma = (i < PARAM_REGISTRY_SIZE - 1) ? "," : "";

        n = snprintf(buf + written, bufLen - written,
            "  {\"name\":\"%s\",\"type\":\"%s\",\"min\":%.4f,\"max\":%.4f,"
            "\"default\":%.4f,\"unit\":\"%s\",\"desc\":\"%s\","
            "\"readonly\":%s,\"reboot\":%s}%s\r\n",
            def.name, typeName,
            (double)def.min, (double)def.max, (double)def.defaultVal,
            def.unit, def.description,
            (def.flags & ParamFlags::READONLY) ? "true" : "false",
            (def.flags & ParamFlags::REQUIRES_REBOOT) ? "true" : "false",
            comma);
        if (n > 0) written += n;
    }

    n = snprintf(buf + written, bufLen - written, "]\r\n");
    if (n > 0) written += n;

    return written;
}

// ============================================================================
// Format `help`
// ============================================================================
int formatHelp(char* buf, size_t bufLen) {
    return snprintf(buf, bufLen,
        "MeltyFC CLI commands:\r\n"
        "  get <name>       — read parameter value\r\n"
        "  set <name> <val> — set parameter value\r\n"
        "  dump             — dump all params (replayable)\r\n"
        "  save             — persist config to flash\r\n"
        "  defaults         — reset all params to defaults\r\n"
        "  list             — list all parameters\r\n"
        "  list json        — list params as JSON (for configurator)\r\n"
        "  status           — live telemetry (omega, RPM, orientation...)\r\n"
        "  cal              — enter calibration mode\r\n"
        "  version          — firmware version\r\n"
        "  help             — this message\r\n"
    );
}

// ============================================================================
// Format `version`
// ============================================================================

#ifndef MELTYFC_VERSION
#define MELTYFC_VERSION "0.1.0-dev"
#endif

#ifndef MELTYFC_TARGET
#define MELTYFC_TARGET "unknown"
#endif

int formatVersion(char* buf, size_t bufLen) {
    return snprintf(buf, bufLen,
        "MeltyFC v%s\r\n"
        "Target: %s\r\n"
        "Schema: v%u\r\n"
        "Params: %zu\r\n",
        MELTYFC_VERSION, MELTYFC_TARGET,
        ConfigData::SCHEMA_VERSION,
        PARAM_REGISTRY_SIZE);
}

// ============================================================================
// Execute `set` — validate and apply
// ============================================================================
const char* executeSet(ConfigData& cfg, const char* name, const char* valueStr) {
    if (name == nullptr) return "Error: missing parameter name\r\n";
    if (valueStr == nullptr) return "Error: missing value\r\n";

    const ParamDef* def = findParam(name);
    if (def == nullptr) return "Error: unknown parameter\r\n";

    if (def->flags & ParamFlags::READONLY) return "Error: parameter is read-only\r\n";

    // Parse value
    char* endptr;
    float value = strtof(valueStr, &endptr);
    if (endptr == valueStr) return "Error: invalid value\r\n";

    // Special handling for bool-like strings
    if (def->type == ParamType::BOOL || def->type == ParamType::UINT8) {
        if (strcmp(valueStr, "on") == 0 || strcmp(valueStr, "ON") == 0 ||
            strcmp(valueStr, "true") == 0 || strcmp(valueStr, "TRUE") == 0) {
            value = 1.0f;
        } else if (strcmp(valueStr, "off") == 0 || strcmp(valueStr, "OFF") == 0 ||
                   strcmp(valueStr, "false") == 0 || strcmp(valueStr, "FALSE") == 0) {
            value = 0.0f;
        }
    }

    if (!setParamFloat(cfg, *def, value)) return "Error: failed to set value\r\n";
    return nullptr;  // Success
}

} // namespace melty
