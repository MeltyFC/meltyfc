// MeltyFC — Config Store Implementation
// Parameter registry + get/set/format by ParamDef.
// See spec §11, §11B.

#include "config_store.hpp"
#include "param_registry.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace melty {

// ============================================================================
// Parameter registry — the single source of truth for all configurable values.
// CLI `list` and future GUI autogenerate from this array.
// ============================================================================

const ParamDef PARAM_REGISTRY[] = {
    // --- Bot geometry & drivetrain ---
    {"NUM_DRIVE_MOTORS", "", "Number of drive motors (1-4)", ParamType::UINT8, ParamFlags::NONE, 1,
     4, 3, PARAM_OFFSET(numDriveMotors)},
    {"MOTOR_ANGLE_0", "deg", "Motor 0 mounting angle", ParamType::FLOAT, ParamFlags::NONE, 0, 360,
     0, PARAM_OFFSET(motorAngle[0])},
    {"MOTOR_ANGLE_1", "deg", "Motor 1 mounting angle", ParamType::FLOAT, ParamFlags::NONE, 0, 360,
     120, PARAM_OFFSET(motorAngle[1])},
    {"MOTOR_ANGLE_2", "deg", "Motor 2 mounting angle", ParamType::FLOAT, ParamFlags::NONE, 0, 360,
     240, PARAM_OFFSET(motorAngle[2])},
    {"MOTOR_ANGLE_3", "deg", "Motor 3 mounting angle", ParamType::FLOAT, ParamFlags::NONE, 0, 360,
     0, PARAM_OFFSET(motorAngle[3])},
    {"SPIN_DIRECTION", "", "Spin direction (0=CW, 1=CCW)", ParamType::UINT8, ParamFlags::NONE, 0, 1,
     0, PARAM_OFFSET(spinDirection)},
    {"MAX_RPM", "rpm", "Maximum spin RPM (governor cap)", ParamType::UINT16, ParamFlags::NONE, 100,
     10000, 3200, PARAM_OFFSET(maxRpm)},
    {"R_INNER", "mm", "Inner accel sensor radius", ParamType::FLOAT, ParamFlags::NONE, 1, 200, 15,
     PARAM_OFFSET(rInner)},
    {"R_OUTER", "mm", "Outer accel sensor radius", ParamType::FLOAT, ParamFlags::NONE, 1, 200, 28,
     PARAM_OFFSET(rOuter)},
    {"R_ICM", "mm", "ICM-42688 radius from spin axis (for low-speed mode)", ParamType::FLOAT,
     ParamFlags::NONE, 1, 200, 10, PARAM_OFFSET(rIcm)},
    {"DR_EFF", "mm", "Effective radius difference (calibrated)", ParamType::FLOAT, ParamFlags::NONE,
     0.1f, 200, 13, PARAM_OFFSET(drEff)},
    {"WHEEL_DIA", "mm", "Drive wheel diameter", ParamType::FLOAT, ParamFlags::NONE, 5, 200, 40,
     PARAM_OFFSET(wheelDia)},
    {"WHEEL_MOUNT_RADIUS", "mm", "Wheel center to spin axis", ParamType::FLOAT, ParamFlags::NONE,
     10, 500, 85, PARAM_OFFSET(wheelMountRadius)},
    {"DRIVE_RATIO", "", "Gear ratio bell:wheel (1.0=direct)", ParamType::FLOAT, ParamFlags::NONE,
     0.1f, 100, 5.0f, PARAM_OFFSET(driveRatio)},
    {"MOTOR_POLES", "", "Motor pole count (for eRPM conversion)", ParamType::UINT8,
     ParamFlags::NONE, 2, 50, 14, PARAM_OFFSET(motorPoles)},

    // --- Control & behavior ---
    {"WINDOW_HALF", "deg", "Translation window half-width", ParamType::FLOAT, ParamFlags::NONE, 5,
     90, 30, PARAM_OFFSET(windowHalf)},
    {"THROTTLE_SPIN_MAX", "", "Max spin throttle (0.0-1.0)", ParamType::FLOAT, ParamFlags::NONE, 0,
     1.0f, 0.75f, PARAM_OFFSET(throttleSpinMax)},
    {"THROTTLE_CAP", "", "Absolute throttle cap (0.0-1.0)", ParamType::FLOAT, ParamFlags::NONE, 0,
     1.0f, 0.90f, PARAM_OFFSET(throttleCap)},
    {"TRIM_RATE_FINE", "deg/s", "Trim rate at min deflection", ParamType::FLOAT, ParamFlags::NONE,
     1, 180, 15, PARAM_OFFSET(trimRateFine)},
    {"TRIM_RATE_MAX", "deg/s", "Trim rate at full deflection", ParamType::FLOAT, ParamFlags::NONE,
     10, 1080, 360, PARAM_OFFSET(trimRateMax)},
    {"TRIM_EXPO", "", "Trim expo (0=linear, 1=max curve)", ParamType::FLOAT, ParamFlags::NONE, 0,
     1.0f, 0.3f, PARAM_OFFSET(trimExpo)},
    {"ORIENT_DEBOUNCE_MS", "ms", "Orientation change debounce", ParamType::UINT16, ParamFlags::NONE,
     10, 1000, 150, PARAM_OFFSET(orientDebounceMs)},
    {"HIT_THRESH_G", "g", "Hit detection threshold", ParamType::FLOAT, ParamFlags::NONE, 5, 400, 50,
     PARAM_OFFSET(hitThreshG)},
    {"GYRO_BLANK_MS", "ms", "Gyro blanking after hit", ParamType::UINT16, ParamFlags::NONE, 10,
     1000, 100, PARAM_OFFSET(gyroBlankMs)},
    {"OMEGA_SLEW_MAX", "rad/s2", "Max omega rate of change", ParamType::FLOAT, ParamFlags::NONE, 10,
     5000, 200, PARAM_OFFSET(omegaSlewMax)},
    {"SLIP_WARN_PCT", "%", "Slip warning threshold", ParamType::FLOAT, ParamFlags::NONE, 1, 100, 25,
     PARAM_OFFSET(slipWarnPct)},
    {"SLIP_WARN_MS", "ms", "Slip warning sustain time", ParamType::UINT16, ParamFlags::NONE, 50,
     5000, 300, PARAM_OFFSET(slipWarnMs)},
    {"LOWSPEED_SWITCH_RPM", "rpm", "Switch to ICM below this RPM", ParamType::UINT16,
     ParamFlags::NONE, 100, 3000, 900, PARAM_OFFSET(lowspeedSwitchRpm)},

    // --- RPM hold governor ---
    {"RPM_HOLD_ENABLED", "", "RPM hold on/off (0=off, 1=on)", ParamType::UINT8, ParamFlags::NONE, 0,
     1, 0, PARAM_OFFSET(rpmHoldEnabled)},
    {"RPM_HOLD_KP", "", "RPM hold P gain", ParamType::FLOAT, ParamFlags::NONE, 0.0001f, 0.1f,
     0.002f, PARAM_OFFSET(rpmHoldKp)},
    {"RPM_HOLD_FF", "", "RPM hold feedforward (base throttle)", ParamType::FLOAT, ParamFlags::NONE,
     0, 1.0f, 0.5f, PARAM_OFFSET(rpmHoldFeedforward)},

    // --- Creep/tank mode ---
    {"CREEP_THRESHOLD_RPM", "rpm", "Enter creep below this RPM", ParamType::UINT16,
     ParamFlags::NONE, 50, 1000, 200, PARAM_OFFSET(creepThresholdRpm)},
    {"CREEP_HYSTERESIS_RPM", "rpm", "Creep exit hysteresis", ParamType::UINT16, ParamFlags::NONE,
     10, 500, 50, PARAM_OFFSET(creepHysteresisRpm)},

    // --- LED ---
    {"LED_COUNT", "", "Number of WS2812 LEDs", ParamType::UINT16, ParamFlags::NONE, 1, 144, 12,
     PARAM_OFFSET(ledCount)},
    {"LED_ARC_WIDTH", "deg", "Beacon arc width", ParamType::FLOAT, ParamFlags::NONE, 5, 180, 45,
     PARAM_OFFSET(ledArcWidth)},

    // --- RC channel map ---
    {"CH_TRANSLATE_X", "", "CRSF channel: translate X (0-based)", ParamType::UINT8,
     ParamFlags::NONE, 0, 15, 0, PARAM_OFFSET(chTranslateX)},
    {"CH_TRANSLATE_Y", "", "CRSF channel: translate Y (0-based)", ParamType::UINT8,
     ParamFlags::NONE, 0, 15, 1, PARAM_OFFSET(chTranslateY)},
    {"CH_SPIN", "", "CRSF channel: spin throttle (0-based)", ParamType::UINT8, ParamFlags::NONE, 0,
     15, 2, PARAM_OFFSET(chSpin)},
    {"CH_TRIM", "", "CRSF channel: heading trim (0-based)", ParamType::UINT8, ParamFlags::NONE, 0,
     15, 3, PARAM_OFFSET(chTrim)},
    {"CH_ARM", "", "CRSF channel: arm switch (0-based)", ParamType::UINT8, ParamFlags::NONE, 0, 15,
     4, PARAM_OFFSET(chArm)},
    {"CH_RESYNC", "", "CRSF channel: heading re-sync (0-based)", ParamType::UINT8, ParamFlags::NONE,
     0, 15, 5, PARAM_OFFSET(chResync)},
    {"CH_CREEP_FORCE", "", "CRSF channel: force creep mode (0-based)", ParamType::UINT8,
     ParamFlags::NONE, 0, 15, 6, PARAM_OFFSET(chCreepForce)},

    // --- Safety ---
    {"FAILSAFE_MS", "ms", "Failsafe timeout (hard floor 500ms)", ParamType::UINT16,
     ParamFlags::FLOOR, 500, 5000, 500, PARAM_OFFSET(failsafeMs)},
    {"WATCHDOG_MS", "ms", "Watchdog timeout", ParamType::UINT16, ParamFlags::NONE, 50, 1000, 200,
     PARAM_OFFSET(watchdogMs)},

    // --- Low voltage ---
    {"LVC_WARN_VOLTS", "V", "Low voltage warning per-cell", ParamType::FLOAT, ParamFlags::NONE,
     2.5f, 4.2f, 3.3f, PARAM_OFFSET(lvcWarnVolts)},
    {"LVC_CRIT_VOLTS", "V", "Low voltage critical per-cell", ParamType::FLOAT, ParamFlags::NONE,
     2.0f, 4.0f, 3.0f, PARAM_OFFSET(lvcCritVolts)},
    {"CELL_COUNT", "", "Battery cell count (0=auto)", ParamType::UINT8, ParamFlags::NONE, 0, 12, 0,
     PARAM_OFFSET(cellCount)},

    // --- Blackbox ---
    {"BLACKBOX_ENABLED", "", "Blackbox logging on/off", ParamType::UINT8, ParamFlags::NONE, 0, 1, 1,
     PARAM_OFFSET(blackboxEnabled)},
    {"BLACKBOX_RATE_HZ", "Hz", "Blackbox log rate", ParamType::UINT16, ParamFlags::NONE, 10, 1000,
     100, PARAM_OFFSET(blackboxRateHz)},

    // --- Translation ---
    {"TRANSLATE_DEADBAND", "", "Translation stick deadband (0.0-1.0)", ParamType::FLOAT,
     ParamFlags::NONE, 0, 0.5f, 0.05f, PARAM_OFFSET(translateDeadband)},

    // --- Resync ---
    {"RESYNC_MIN_DEFLECT", "", "Min stick deflection for re-sync (0.0-1.0)", ParamType::FLOAT,
     ParamFlags::NONE, 0.1f, 1.0f, 0.3f, PARAM_OFFSET(resyncMinDeflection)},
    {"RESYNC_AVERAGE_MS", "ms", "Stick angle averaging window", ParamType::UINT16, ParamFlags::NONE,
     10, 1000, 100, PARAM_OFFSET(resyncAverageMs)},
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
    const uint8_t* ptr = base + def.offset;

    // All reads use memcpy to avoid misaligned access on packed structs.
    // Cortex-M4 FPU traps on misaligned VLDR/VSTR — reinterpret_cast crashes.
    switch (def.type) {
    case ParamType::UINT8:
        return static_cast<float>(*ptr);
    case ParamType::UINT16: {
        uint16_t v;
        memcpy(&v, ptr, sizeof(v));
        return static_cast<float>(v);
    }
    case ParamType::UINT32: {
        uint32_t v;
        memcpy(&v, ptr, sizeof(v));
        return static_cast<float>(v);
    }
    case ParamType::INT8:
        return static_cast<float>(*reinterpret_cast<const int8_t*>(ptr));
    case ParamType::INT16: {
        int16_t v;
        memcpy(&v, ptr, sizeof(v));
        return static_cast<float>(v);
    }
    case ParamType::INT32: {
        int32_t v;
        memcpy(&v, ptr, sizeof(v));
        return static_cast<float>(v);
    }
    case ParamType::FLOAT: {
        float v;
        memcpy(&v, ptr, sizeof(v));
        return v;
    }
    case ParamType::BOOL:
        return (*ptr) ? 1.0f : 0.0f;
    case ParamType::ENUM:
        return static_cast<float>(*ptr);
    }
    return 0.0f;
}

// ============================================================================
// Set a parameter value from float, with type conversion + clamping
// Returns false if readonly or value out of range after floor enforcement
// ============================================================================
bool setParamFloat(ConfigData& cfg, const ParamDef& def, float value) {
    if (def.flags & ParamFlags::READONLY)
        return false;

    // Clamp to [min, max]
    if (value < def.min) {
        if (def.flags & ParamFlags::FLOOR) {
            value = def.min; // Floor enforcement: silently clamp
        } else {
            value = def.min;
        }
    }
    if (value > def.max)
        value = def.max;

    uint8_t* base = reinterpret_cast<uint8_t*>(&cfg);
    uint8_t* ptr = base + def.offset;

    // All writes use memcpy to avoid misaligned access on packed structs.
    // See getParamFloat comment for rationale.
    switch (def.type) {
    case ParamType::UINT8:
        *ptr = static_cast<uint8_t>(value);
        break;
    case ParamType::UINT16: {
        uint16_t v = static_cast<uint16_t>(value);
        memcpy(ptr, &v, sizeof(v));
        break;
    }
    case ParamType::UINT32: {
        uint32_t v = static_cast<uint32_t>(value);
        memcpy(ptr, &v, sizeof(v));
        break;
    }
    case ParamType::INT8:
        *reinterpret_cast<int8_t*>(ptr) = static_cast<int8_t>(value);
        break;
    case ParamType::INT16: {
        int16_t v = static_cast<int16_t>(value);
        memcpy(ptr, &v, sizeof(v));
        break;
    }
    case ParamType::INT32: {
        int32_t v = static_cast<int32_t>(value);
        memcpy(ptr, &v, sizeof(v));
        break;
    }
    case ParamType::FLOAT:
        memcpy(ptr, &value, sizeof(value));
        break;
    case ParamType::BOOL:
        *ptr = (value >= 0.5f) ? 1 : 0;
        break;
    case ParamType::ENUM:
        *ptr = static_cast<uint8_t>(value);
        break;
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

    if (oldData == nullptr || oldSize < sizeof(uint16_t))
        return false;

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
        if (crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320;
        else
            crc >>= 1;
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

// ============================================================================
// Cross-param validation (Round 2 A1, A4, A8)
// Fixes issues in-place (clamp/adjust) and reports what was wrong.
// ============================================================================
ConfigValidationResult validateConfig(ConfigData& cfg) {
    ConfigValidationResult result = {};

    // A1: THROTTLE_SPIN_MAX must be <= THROTTLE_CAP - margin
    constexpr float SPIN_CAP_MARGIN = 0.05f;
    if (cfg.throttleSpinMax > cfg.throttleCap - SPIN_CAP_MARGIN) {
        cfg.throttleSpinMax = cfg.throttleCap - SPIN_CAP_MARGIN;
        result.spinMaxExceedsCap = true;
        result.issueCount++;
    }

    // R_INNER must be < R_OUTER
    if (cfg.rInner >= cfg.rOuter) {
        cfg.rInner = cfg.rOuter - 1.0f;
        result.innerGtOuter = true;
        result.issueCount++;
    }

    // LVC_WARN must be > LVC_CRIT
    if (cfg.lvcWarnVolts <= cfg.lvcCritVolts) {
        cfg.lvcWarnVolts = cfg.lvcCritVolts + 0.3f;
        result.lvcWarnBelowCrit = true;
        result.issueCount++;
    }

    // WINDOW_HALF must be <= 90°
    if (cfg.windowHalf > 90.0f) {
        cfg.windowHalf = 90.0f;
        result.windowHalfTooLarge = true;
        result.issueCount++;
    }

    // A4: Channel collision check
    uint8_t channels[] = {cfg.chArm,  cfg.chSpin,   cfg.chTranslateX, cfg.chTranslateY,
                          cfg.chTrim, cfg.chResync, cfg.chCreepForce};
    constexpr int NUM_CH = sizeof(channels) / sizeof(channels[0]);
    for (int i = 0; i < NUM_CH; i++) {
        for (int j = i + 1; j < NUM_CH; j++) {
            if (channels[i] == channels[j]) {
                result.channelCollision = true;
                result.issueCount++;
                break;
            }
        }
        if (result.channelCollision)
            break;
    }

    // A8: NUM_DRIVE_MOTORS must be 2-4 (1 is broken for creep/translation)
    if (cfg.numDriveMotors < 2 || cfg.numDriveMotors > 4) {
        if (cfg.numDriveMotors < 2)
            cfg.numDriveMotors = 2;
        if (cfg.numDriveMotors > 4)
            cfg.numDriveMotors = 4;
        result.numMotorsInvalid = true;
        result.issueCount++;
    }

    // R15-3: ω²r-vs-sensor — legal configs must not rail the H3LIS ±400g accelerometer.
    // centripetal_g = (maxRpm × 2π/60)² × rOuter_m / 9.81
    // Margin 0.85 for vibration headroom → limit = 400 × 0.85 = 340g
    {
        constexpr float PI = 3.14159265358979f;
        constexpr float G_ACCEL = 9.81f;
        constexpr float H3LIS_LIMIT_G = 400.0f;
        constexpr float MARGIN = 0.85f;
        float omega = static_cast<float>(cfg.maxRpm) * 2.0f * PI / 60.0f;
        float rOuterM = cfg.rOuter * 0.001f;  // mm → m
        float centripG = (omega * omega * rOuterM) / G_ACCEL;
        if (centripG > H3LIS_LIMIT_G * MARGIN) {
            result.accelSaturation = true;
            result.issueCount++;
            // Don't auto-fix — reject. The user must reduce maxRpm or rOuter.
        }
    }

    // R15-4: Window-vs-sampling floor — prevents stochastically dead translation.
    // At LOOP_HZ, deg_per_sample = maxRpm × 360 / 60 / LOOP_HZ = maxRpm × 6 / LOOP_HZ.
    // Rule: windowHalf ≥ 1.5 × deg_per_sample, otherwise the boost window is hit
    // some revs and missed others → pulsing/dead translation with no error.
    {
        constexpr float LOOP_HZ = 2000.0f;
        float degPerSample = static_cast<float>(cfg.maxRpm) * 6.0f / LOOP_HZ;
        float minWindow = 1.5f * degPerSample;
        if (cfg.windowHalf < minWindow) {
            result.windowSamplingDead = true;
            result.issueCount++;
            // Don't auto-fix — reject. User must widen window or lower maxRpm.
        }
    }

    return result;
}

// ============================================================================
// R15-5: Load-path per-field clamp
// ============================================================================
uint8_t clampConfigToRegistry(ConfigData& cfg) {
    uint8_t clamped = 0;
    for (size_t i = 0; i < PARAM_REGISTRY_SIZE; i++) {
        const ParamDef& def = PARAM_REGISTRY[i];
        if (def.flags & ParamFlags::READONLY)
            continue;

        float val = getParamFloat(cfg, def);
        float original = val;

        // Clamp to [min, max]
        if (val < def.min) {
            val = def.min;  // FLOOR-flagged params clamp UP here naturally
        }
        if (val > def.max) {
            val = def.max;
        }

        if (val != original) {
            (void)setParamFloat(cfg, def, val); // G-1: benign — readonly already checked above
            clamped++;
        }
    }
    return clamped;
}

} // namespace melty
