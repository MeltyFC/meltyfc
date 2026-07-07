// MeltyFC — Config Store Implementation
// Parameter registry + get/set/format by ParamDef.
// See spec §11, §11B.

#include "config_store.hpp"
#include "param_registry.h"
#include <cstring>
#include <cstdio>
#include <cmath>

namespace melty {

// ============================================================================
// Parameter registry — the single source of truth for all configurable values.
// CLI `list` and future GUI autogenerate from this array.
// ============================================================================

const ParamDef PARAM_REGISTRY[] = {
    // --- Bot geometry & drivetrain ---
    {"NUM_DRIVE_MOTORS",   "",    "Number of drive motors (1-4)",
        ParamType::UINT8, ParamFlags::NONE, 1, 4, 3, PARAM_OFFSET(numDriveMotors)},
    {"MOTOR_ANGLE_0",      "deg", "Motor 0 mounting angle",
        ParamType::FLOAT, ParamFlags::NONE, 0, 360, 0,   PARAM_OFFSET(motorAngle[0])},
    {"MOTOR_ANGLE_1",      "deg", "Motor 1 mounting angle",
        ParamType::FLOAT, ParamFlags::NONE, 0, 360, 120, PARAM_OFFSET(motorAngle[1])},
    {"MOTOR_ANGLE_2",      "deg", "Motor 2 mounting angle",
        ParamType::FLOAT, ParamFlags::NONE, 0, 360, 240, PARAM_OFFSET(motorAngle[2])},
    {"MOTOR_ANGLE_3",      "deg", "Motor 3 mounting angle",
        ParamType::FLOAT, ParamFlags::NONE, 0, 360, 0,   PARAM_OFFSET(motorAngle[3])},
    {"SPIN_DIRECTION",     "",    "Spin direction (0=CW, 1=CCW)",
        ParamType::UINT8, ParamFlags::NONE, 0, 1, 0, PARAM_OFFSET(spinDirection)},
    {"MAX_RPM",            "rpm", "Maximum spin RPM (governor cap)",
        ParamType::UINT16, ParamFlags::NONE, 100, 10000, 3200, PARAM_OFFSET(maxRpm)},
    {"R_INNER",            "mm",  "Inner accel sensor radius",
        ParamType::FLOAT, ParamFlags::NONE, 1, 200, 15, PARAM_OFFSET(rInner)},
    {"R_OUTER",            "mm",  "Outer accel sensor radius",
        ParamType::FLOAT, ParamFlags::NONE, 1, 200, 28, PARAM_OFFSET(rOuter)},
    {"DR_EFF",             "mm",  "Effective radius difference (calibrated)",
        ParamType::FLOAT, ParamFlags::NONE, 0.1f, 200, 13, PARAM_OFFSET(drEff)},
    {"WHEEL_DIA",          "mm",  "Drive wheel diameter",
        ParamType::FLOAT, ParamFlags::NONE, 5, 200, 40, PARAM_OFFSET(wheelDia)},
    {"WHEEL_MOUNT_RADIUS", "mm",  "Wheel center to spin axis",
        ParamType::FLOAT, ParamFlags::NONE, 10, 500, 85, PARAM_OFFSET(wheelMountRadius)},
    {"DRIVE_RATIO",        "",    "Gear ratio bell:wheel (1.0=direct)",
        ParamType::FLOAT, ParamFlags::NONE, 0.1f, 100, 5.0f, PARAM_OFFSET(driveRatio)},
    {"MOTOR_POLES",        "",    "Motor pole count (for eRPM conversion)",
        ParamType::UINT8, ParamFlags::NONE, 2, 50, 14, PARAM_OFFSET(motorPoles)},

    // --- Control & behavior ---
    {"WINDOW_HALF",        "deg", "Translation window half-width",
        ParamType::FLOAT, ParamFlags::NONE, 5, 90, 30, PARAM_OFFSET(windowHalf)},
    {"THROTTLE_SPIN_MAX",  "",    "Max spin throttle (0.0-1.0)",
        ParamType::FLOAT, ParamFlags::NONE, 0, 1.0f, 0.75f, PARAM_OFFSET(throttleSpinMax)},
    {"THROTTLE_CAP",       "",    "Absolute throttle cap (0.0-1.0)",
        ParamType::FLOAT, ParamFlags::NONE, 0, 1.0f, 0.90f, PARAM_OFFSET(throttleCap)},
    {"TRIM_RATE_FINE",     "deg/s","Trim rate at min deflection",
        ParamType::FLOAT, ParamFlags::NONE, 1, 180, 15, PARAM_OFFSET(trimRateFine)},
    {"TRIM_RATE_MAX",      "deg/s","Trim rate at full deflection",
        ParamType::FLOAT, ParamFlags::NONE, 10, 1080, 360, PARAM_OFFSET(trimRateMax)},
    {"TRIM_EXPO",          "",    "Trim expo (0=linear, 1=max curve)",
        ParamType::FLOAT, ParamFlags::NONE, 0, 1.0f, 0.3f, PARAM_OFFSET(trimExpo)},
    {"ORIENT_DEBOUNCE_MS", "ms",  "Orientation change debounce",
        ParamType::UINT16, ParamFlags::NONE, 10, 1000, 150, PARAM_OFFSET(orientDebounceMs)},
    {"HIT_THRESH_G",       "g",   "Hit detection threshold",
        ParamType::FLOAT, ParamFlags::NONE, 5, 400, 50, PARAM_OFFSET(hitThreshG)},
    {"GYRO_BLANK_MS",      "ms",  "Gyro blanking after hit",
        ParamType::UINT16, ParamFlags::NONE, 10, 1000, 100, PARAM_OFFSET(gyroBlankMs)},
    {"OMEGA_SLEW_MAX",     "rad/s2","Max omega rate of change",
        ParamType::FLOAT, ParamFlags::NONE, 10, 5000, 200, PARAM_OFFSET(omegaSlewMax)},
    {"SLIP_WARN_PCT",      "%",   "Slip warning threshold",
        ParamType::FLOAT, ParamFlags::NONE, 1, 100, 25, PARAM_OFFSET(slipWarnPct)},
    {"SLIP_WARN_MS",       "ms",  "Slip warning sustain time",
        ParamType::UINT16, ParamFlags::NONE, 50, 5000, 300, PARAM_OFFSET(slipWarnMs)},
    {"LOWSPEED_SWITCH_RPM","rpm", "Switch to ICM below this RPM",
        ParamType::UINT16, ParamFlags::NONE, 100, 3000, 900, PARAM_OFFSET(lowspeedSwitchRpm)},

    // --- RPM hold governor ---
    {"RPM_HOLD_ENABLED",   "",    "RPM hold on/off (0=off, 1=on)",
        ParamType::UINT8, ParamFlags::NONE, 0, 1, 0, PARAM_OFFSET(rpmHoldEnabled)},
    {"RPM_HOLD_KP",        "",    "RPM hold P gain",
        ParamType::FLOAT, ParamFlags::NONE, 0.0001f, 0.1f, 0.002f, PARAM_OFFSET(rpmHoldKp)},
    {"RPM_HOLD_FF",        "",    "RPM hold feedforward (base throttle)",
        ParamType::FLOAT, ParamFlags::NONE, 0, 1.0f, 0.5f, PARAM_OFFSET(rpmHoldFeedforward)},

    // --- Creep/tank mode ---
    {"CREEP_THRESHOLD_RPM","rpm", "Enter creep below this RPM",
        ParamType::UINT16, ParamFlags::NONE, 50, 1000, 200, PARAM_OFFSET(creepThresholdRpm)},
    {"CREEP_HYSTERESIS_RPM","rpm","Creep exit hysteresis",
        ParamType::UINT16, ParamFlags::NONE, 10, 500, 50, PARAM_OFFSET(creepHysteresisRpm)},

    // --- LED ---
    {"LED_COUNT",          "",    "Number of WS2812 LEDs",
        ParamType::UINT16, ParamFlags::NONE, 1, 144, 12, PARAM_OFFSET(ledCount)},
    {"LED_ARC_WIDTH",      "deg", "Beacon arc width",
        ParamType::FLOAT, ParamFlags::NONE, 5, 180, 45, PARAM_OFFSET(ledArcWidth)},

    // --- RC channel map ---
    {"CH_TRANSLATE_X",     "",    "CRSF channel: translate X (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 0, PARAM_OFFSET(chTranslateX)},
    {"CH_TRANSLATE_Y",     "",    "CRSF channel: translate Y (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 1, PARAM_OFFSET(chTranslateY)},
    {"CH_SPIN",            "",    "CRSF channel: spin throttle (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 2, PARAM_OFFSET(chSpin)},
    {"CH_TRIM",            "",    "CRSF channel: heading trim (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 3, PARAM_OFFSET(chTrim)},
    {"CH_ARM",             "",    "CRSF channel: arm switch (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 4, PARAM_OFFSET(chArm)},
    {"CH_RESYNC",          "",    "CRSF channel: heading re-sync (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 5, PARAM_OFFSET(chResync)},
    {"CH_CREEP_FORCE",     "",    "CRSF channel: force creep mode (0-based)",
        ParamType::UINT8, ParamFlags::NONE, 0, 15, 6, PARAM_OFFSET(chCreepForce)},

    // --- Safety ---
    {"FAILSAFE_MS",        "ms",  "Failsafe timeout (hard floor 500ms)",
        ParamType::UINT16, ParamFlags::FLOOR, 500, 5000, 500, PARAM_OFFSET(failsafeMs)},
    {"WATCHDOG_MS",        "ms",  "Watchdog timeout",
        ParamType::UINT16, ParamFlags::NONE, 50, 1000, 200, PARAM_OFFSET(watchdogMs)},

    // --- Low voltage ---
    {"LVC_WARN_VOLTS",     "V",   "Low voltage warning per-cell",
        ParamType::FLOAT, ParamFlags::NONE, 2.5f, 4.2f, 3.3f, PARAM_OFFSET(lvcWarnVolts)},
    {"LVC_CRIT_VOLTS",     "V",   "Low voltage critical per-cell",
        ParamType::FLOAT, ParamFlags::NONE, 2.0f, 4.0f, 3.0f, PARAM_OFFSET(lvcCritVolts)},
    {"CELL_COUNT",         "",    "Battery cell count (0=auto)",
        ParamType::UINT8, ParamFlags::NONE, 0, 12, 0, PARAM_OFFSET(cellCount)},

    // --- Blackbox ---
    {"BLACKBOX_ENABLED",   "",    "Blackbox logging on/off",
        ParamType::UINT8, ParamFlags::NONE, 0, 1, 1, PARAM_OFFSET(blackboxEnabled)},
    {"BLACKBOX_RATE_HZ",   "Hz",  "Blackbox log rate",
        ParamType::UINT16, ParamFlags::NONE, 10, 1000, 100, PARAM_OFFSET(blackboxRateHz)},

    // --- Translation ---
    {"TRANSLATE_DEADBAND", "",    "Translation stick deadband (0.0-1.0)",
        ParamType::FLOAT, ParamFlags::NONE, 0, 0.5f, 0.05f, PARAM_OFFSET(translateDeadband)},

    // --- Resync ---
    {"RESYNC_MIN_DEFLECT", "",    "Min stick deflection for re-sync (0.0-1.0)",
        ParamType::FLOAT, ParamFlags::NONE, 0.1f, 1.0f, 0.3f, PARAM_OFFSET(resyncMinDeflection)},
    {"RESYNC_AVERAGE_MS",  "ms",  "Stick angle averaging window",
        ParamType::UINT16, ParamFlags::NONE, 10, 1000, 100, PARAM_OFFSET(resyncAverageMs)},
};

const size_t PARAM_REGISTRY_SIZE = sizeof(PARAM_REGISTRY) / sizeof(PARAM_REGISTRY[0]);

// ============================================================================
// Lookup by name (case-sensitive)
// ============================================================================
const ParamDef* findParam(const char* name) {
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE; i++) {
        if (strcmp(PARAM_REGISTRY[i].name, name) == 0) {
            return &PARAM_REGISTRY[i];
        }
    }
    return nullptr;
}

// ============================================================================
// Get a parameter value as float from ConfigData by offset + type
// ============================================================================
float getParamFloat(const ConfigData& cfg, const ParamDef& def) {
    const uint8_t* base = reinterpret_cast<const uint8_t*>(&cfg);
    const void* ptr = base + def.offset;

    switch (def.type) {
        case ParamType::UINT8:  return static_cast<float>(*reinterpret_cast<const uint8_t*>(ptr));
        case ParamType::UINT16: return static_cast<float>(*reinterpret_cast<const uint16_t*>(ptr));
        case ParamType::UINT32: return static_cast<float>(*reinterpret_cast<const uint32_t*>(ptr));
        case ParamType::INT8:   return static_cast<float>(*reinterpret_cast<const int8_t*>(ptr));
        case ParamType::INT16:  return static_cast<float>(*reinterpret_cast<const int16_t*>(ptr));
        case ParamType::INT32:  return static_cast<float>(*reinterpret_cast<const int32_t*>(ptr));
        case ParamType::FLOAT:  return *reinterpret_cast<const float*>(ptr);
        case ParamType::BOOL:   return *reinterpret_cast<const uint8_t*>(ptr) ? 1.0f : 0.0f;
        case ParamType::ENUM:   return static_cast<float>(*reinterpret_cast<const uint8_t*>(ptr));
    }
    return 0.0f;
}

// ============================================================================
// Set a parameter value from float, with type conversion + clamping
// Returns false if readonly or value out of range after floor enforcement
// ============================================================================
bool setParamFloat(ConfigData& cfg, const ParamDef& def, float value) {
    if (def.flags & ParamFlags::READONLY) return false;

    // Clamp to [min, max]
    if (value < def.min) {
        if (def.flags & ParamFlags::FLOOR) {
            value = def.min;  // Floor enforcement: silently clamp
        } else {
            value = def.min;
        }
    }
    if (value > def.max) value = def.max;

    uint8_t* base = reinterpret_cast<uint8_t*>(&cfg);
    void* ptr = base + def.offset;

    switch (def.type) {
        case ParamType::UINT8:  *reinterpret_cast<uint8_t*>(ptr)  = static_cast<uint8_t>(value); break;
        case ParamType::UINT16: *reinterpret_cast<uint16_t*>(ptr) = static_cast<uint16_t>(value); break;
        case ParamType::UINT32: *reinterpret_cast<uint32_t*>(ptr) = static_cast<uint32_t>(value); break;
        case ParamType::INT8:   *reinterpret_cast<int8_t*>(ptr)   = static_cast<int8_t>(value); break;
        case ParamType::INT16:  *reinterpret_cast<int16_t*>(ptr)  = static_cast<int16_t>(value); break;
        case ParamType::INT32:  *reinterpret_cast<int32_t*>(ptr)  = static_cast<int32_t>(value); break;
        case ParamType::FLOAT:  *reinterpret_cast<float*>(ptr)    = value; break;
        case ParamType::BOOL:   *reinterpret_cast<uint8_t*>(ptr)  = (value >= 0.5f) ? 1 : 0; break;
        case ParamType::ENUM:   *reinterpret_cast<uint8_t*>(ptr)  = static_cast<uint8_t>(value); break;
    }
    return true;
}

// ============================================================================
// Format a parameter value as string for CLI display
// Returns number of chars written (excluding null)
// ============================================================================
int formatParam(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen) {
    float val = getParamFloat(cfg, def);

    switch (def.type) {
        case ParamType::UINT8:
        case ParamType::UINT16:
        case ParamType::UINT32:
        case ParamType::ENUM:
            return snprintf(buf, bufLen, "%u", static_cast<unsigned>(val));
        case ParamType::INT8:
        case ParamType::INT16:
        case ParamType::INT32:
            return snprintf(buf, bufLen, "%d", static_cast<int>(val));
        case ParamType::FLOAT:
            // Use minimal precision: strip trailing zeros
            if (fabsf(val - roundf(val)) < 0.0001f) {
                return snprintf(buf, bufLen, "%.0f", static_cast<double>(val));
            } else if (fabsf(val * 10.0f - roundf(val * 10.0f)) < 0.001f) {
                return snprintf(buf, bufLen, "%.1f", static_cast<double>(val));
            } else if (fabsf(val * 100.0f - roundf(val * 100.0f)) < 0.01f) {
                return snprintf(buf, bufLen, "%.2f", static_cast<double>(val));
            } else {
                return snprintf(buf, bufLen, "%.4f", static_cast<double>(val));
            }
        case ParamType::BOOL:
            return snprintf(buf, bufLen, "%s", val >= 0.5f ? "ON" : "OFF");
    }
    return 0;
}

// ============================================================================
// Schema migration — load old config, preserve matching fields, default rest
// ============================================================================
bool migrateConfig(const uint8_t* oldData, size_t oldSize, ConfigData& out) {
    // Always start with defaults
    out = ConfigData{};

    if (oldData == nullptr || oldSize < sizeof(uint16_t)) return false;

    uint16_t oldVersion;
    memcpy(&oldVersion, oldData, sizeof(uint16_t));

    if (oldVersion == ConfigData::SCHEMA_VERSION && oldSize == sizeof(ConfigData)) {
        // Same version, direct copy
        memcpy(&out, oldData, sizeof(ConfigData));
        return true;
    }

    // Version mismatch: keep fields that fit, default the rest
    // For v1, this is just "defaults if version doesn't match"
    // Future: field-by-field migration based on version history
    return false;
}

// ============================================================================
// CRC32 for config integrity (simple implementation)
// ============================================================================
static uint32_t crc32_byte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
        else crc >>= 1;
    }
    return crc;
}

uint32_t computeConfigCrc(const ConfigData& cfg) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&cfg);
    // CRC everything except the crc32 field itself (last 4 bytes)
    size_t len = sizeof(ConfigData) - sizeof(uint32_t);
    for (size_t i = 0; i < len; i++) {
        crc = crc32_byte(crc, data[i]);
    }
    return crc ^ 0xFFFFFFFF;
}

} // namespace melty
