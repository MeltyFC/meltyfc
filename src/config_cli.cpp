// MeltyFC — USB CDC CLI Implementation
// DI-01 through DI-05 fixes: transactional config, trailing garbage rejection,
// bool parsing before numeric, NaN/Inf rejection, explicit clamp reporting.

#include "config_cli.hpp"
#include "config_store.hpp"
#include "param_registry.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace melty {

// findParam() and formatParam() are in config_store.cpp

// ============================================================================
// CLI parser
// ============================================================================

CliParsed parseCli(char* input) {
    CliParsed result = {CliCommand::NONE, nullptr, nullptr};

    if (input == nullptr)
        return result;

    // Strip trailing whitespace/newline
    size_t len = strlen(input);
    while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r' || input[len - 1] == ' '))
        input[--len] = '\0';

    if (len == 0)
        return result;

    // Extract command word
    char* cmd = input;
    char* rest = nullptr;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == ' ') {
            input[i] = '\0';
            rest = &input[i + 1];
            // Skip leading whitespace in args
            while (*rest == ' ')
                rest++;
            break;
        }
    }

    // Case-insensitive command matching
    auto ciEq = [](const char* a, const char* b) -> bool {
        while (*a && *b) {
            char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
            char cb = (*b >= 'a' && *b <= 'z') ? (*b - 32) : *b;
            if (ca != cb)
                return false;
            a++;
            b++;
        }
        return *a == '\0' && *b == '\0';
    };

    if (ciEq(cmd, "get")) {
        result.command = CliCommand::GET;
        result.arg1 = rest;
    } else if (ciEq(cmd, "set")) {
        result.command = CliCommand::SET;
        if (rest) {
            result.arg1 = rest;
            for (char* p = rest; *p; p++) {
                if (*p == ' ') {
                    *p = '\0';
                    result.arg2 = p + 1;
                    while (*result.arg2 == ' ')
                        result.arg2++;
                    break;
                }
            }
        }
    } else if (ciEq(cmd, "dump")) {
        result.command = CliCommand::DUMP;
    } else if (ciEq(cmd, "save")) {
        result.command = CliCommand::SAVE;
    } else if (ciEq(cmd, "defaults")) {
        result.command = CliCommand::DEFAULTS;
    } else if (ciEq(cmd, "list")) {
        if (rest && ciEq(rest, "json")) {
            result.command = CliCommand::LIST_JSON;
        } else {
            result.command = CliCommand::LIST;
        }
    } else if (ciEq(cmd, "status")) {
        result.command = CliCommand::STATUS;
    } else if (ciEq(cmd, "cal")) {
        result.command = CliCommand::CAL;
    } else if (ciEq(cmd, "version")) {
        result.command = CliCommand::VERSION;
    } else if (ciEq(cmd, "help") || ciEq(cmd, "?")) {
        result.command = CliCommand::HELP;
    } else {
        result.command = CliCommand::UNKNOWN;
        result.arg1 = cmd;
    }

    return result;
}

// ============================================================================
// CLI output formatters
// ============================================================================

int formatGet(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen) {
    char valBuf[32];
    formatParam(cfg, def, valBuf, sizeof(valBuf));
    return snprintf(buf, bufLen, "%s = %s %s\r\n", def.name, valBuf, def.unit);
}

int formatDump(const ConfigData& cfg, char* buf, size_t bufLen) {
    int written = 0;
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 64; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        char valBuf[32];
        formatParam(cfg, def, valBuf, sizeof(valBuf));
        int n = snprintf(buf + written, bufLen - written, "set %s %s\r\n", def.name, valBuf);
        if (n > 0)
            written += n;
    }
    // DI-19: End marker for replayability — truncated dumps are detectable
    int n = snprintf(buf + written, bufLen - written, "# END MeltyFC config dump\r\n");
    if (n > 0)
        written += n;
    return written;
}

int formatList(char* buf, size_t bufLen) {
    int written = 0;
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 128; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        char minBuf[16], maxBuf[16];
        if (def.type == ParamType::FLOAT) {
            snprintf(minBuf, sizeof(minBuf), "%.2f", (double)def.min);
            snprintf(maxBuf, sizeof(maxBuf), "%.2f", (double)def.max);
        } else {
            snprintf(minBuf, sizeof(minBuf), "%d", (int)def.min);
            snprintf(maxBuf, sizeof(maxBuf), "%d", (int)def.max);
        }
        int n = snprintf(buf + written, bufLen - written, "%-24s %s [%s..%s] %s — %s\r\n", def.name,
                         def.unit, minBuf, maxBuf, (def.flags & ParamFlags::READONLY) ? "[RO]" : "",
                         def.description);
        if (n > 0)
            written += n;
    }
    return written;
}

// DI-20: Tiny JSON string escaper
static int jsonEscapeString(const char* src, char* dst, int dstLen) {
    int pos = 0;
    dst[pos++] = '"';
    for (const char* p = src; *p && pos < dstLen - 2; p++) {
        switch (*p) {
        case '"':
            if (pos + 2 < dstLen) {
                dst[pos++] = '\\';
                dst[pos++] = '"';
            }
            break;
        case '\\':
            if (pos + 2 < dstLen) {
                dst[pos++] = '\\';
                dst[pos++] = '\\';
            }
            break;
        case '\n':
            if (pos + 2 < dstLen) {
                dst[pos++] = '\\';
                dst[pos++] = 'n';
            }
            break;
        case '\r':
            break; // Skip
        default:
            if (*p >= 0x20) // Printable only
                dst[pos++] = *p;
            break;
        }
    }
    dst[pos++] = '"';
    dst[pos] = '\0';
    return pos;
}

int formatListJson(char* buf, size_t bufLen) {
    int written = 0;
    int n;

    static const char* typeNames[] = {"uint8", "uint16", "uint32", "int8", "int16",
                                      "int32", "float",  "bool",   "enum"};

    n = snprintf(buf + written, bufLen - written, "[\r\n");
    if (n > 0)
        written += n;

    for (size_t i = 0; i < PARAM_REGISTRY_SIZE && written < (int)bufLen - 256; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        const char* typeName = (uint8_t)def.type < 9 ? typeNames[(uint8_t)def.type] : "unknown";
        const char* comma = (i < PARAM_REGISTRY_SIZE - 1) ? "," : "";

        // DI-20: Escape description for JSON safety
        char descEsc[128];
        jsonEscapeString(def.description, descEsc, sizeof(descEsc));

        n = snprintf(buf + written, bufLen - written,
                     "  {\"name\":\"%s\",\"type\":\"%s\",\"min\":%.4f,\"max\":%.4f,"
                     "\"default\":%.4f,\"unit\":\"%s\",\"desc\":%s,"
                     "\"readonly\":%s,\"reboot\":%s}%s\r\n",
                     def.name, typeName, (double)def.min, (double)def.max, (double)def.defaultVal,
                     def.unit, descEsc, // Already quoted by escaper
                     (def.flags & ParamFlags::READONLY) ? "true" : "false",
                     (def.flags & ParamFlags::REQUIRES_REBOOT) ? "true" : "false", comma);
        if (n > 0)
            written += n;
    }

    n = snprintf(buf + written, bufLen - written, "]\r\n");
    if (n > 0)
        written += n;

    return written;
}

int formatHelp(char* buf, size_t bufLen) {
    return snprintf(buf, bufLen,
                    "MeltyFC CLI Commands:\r\n"
                    "  get <name>       — read parameter value\r\n"
                    "  set <name> <val> — set parameter value\r\n"
                    "  dump             — export all params (replayable)\r\n"
                    "  save             — persist to flash\r\n"
                    "  defaults         — reset all to factory values\r\n"
                    "  list             — list all params with ranges\r\n"
                    "  list json        — list params as JSON (for GUI)\r\n"
                    "  status           — live telemetry snapshot\r\n"
                    "  cal              — enter calibration mode\r\n"
                    "  version          — firmware version\r\n"
                    "  help / ?         — this text\r\n");
}

int formatVersion(char* buf, size_t bufLen) {
    return snprintf(buf, bufLen, "MeltyFC v0.1.0-dev (schema %d)\r\n", ConfigData::SCHEMA_VERSION);
}

// ============================================================================
// CLI set — DI-01 through DI-05 rewritten
// ============================================================================

const char* executeSet(ConfigData& cfg, const char* name, const char* valueStr) {
    if (name == nullptr)
        return "Error: missing parameter name\r\n";
    if (valueStr == nullptr)
        return "Error: missing value\r\n";

    const ParamDef* def = findParam(name);
    if (def == nullptr)
        return "Error: unknown parameter\r\n";

    if (def->flags & ParamFlags::READONLY)
        return "Error: parameter is read-only\r\n";

    float value;

    // DI-03: Handle bool-like strings BEFORE numeric parsing
    if (def->type == ParamType::BOOL) {
        if (strcmp(valueStr, "on") == 0 || strcmp(valueStr, "ON") == 0 ||
            strcmp(valueStr, "true") == 0 || strcmp(valueStr, "TRUE") == 0 ||
            strcmp(valueStr, "1") == 0) {
            value = 1.0f;
        } else if (strcmp(valueStr, "off") == 0 || strcmp(valueStr, "OFF") == 0 ||
                   strcmp(valueStr, "false") == 0 || strcmp(valueStr, "FALSE") == 0 ||
                   strcmp(valueStr, "0") == 0) {
            value = 0.0f;
        } else {
            return "Error: bool values must be on/off/true/false/0/1\r\n";
        }
    } else {
        // Numeric parsing
        char* endptr = nullptr;
        value = strtof(valueStr, &endptr);

        // DI-02: Reject if nothing was parsed
        if (endptr == valueStr)
            return "Error: invalid value\r\n";

        // DI-02: Reject trailing non-whitespace garbage
        while (*endptr == ' ' || *endptr == '\t')
            endptr++;
        if (*endptr != '\0')
            return "Error: trailing characters in value\r\n";

        // DI-17: Reject NaN/Inf
        if (!std::isfinite(value))
            return "Error: non-finite value (NaN/Inf)\r\n";
    }

    // DI-01: Check if value is in range BEFORE setting
    if (value < def->min || value > def->max) {
        return "Error: value out of range\r\n";
    }

    // DI-04/DI-05: Transactional validation — apply to candidate first
    ConfigData candidate = cfg;
    if (!setParamFloat(candidate, *def, value))
        return "Error: failed to set value\r\n";

    // Cross-parameter validation
    auto result = validateConfig(candidate);
    if (result.issueCount > 0) {
        return "Error: cross-parameter conflict (check related params)\r\n";
    }

    cfg = candidate;
    return nullptr; // Success — exact value accepted
}

} // namespace melty
