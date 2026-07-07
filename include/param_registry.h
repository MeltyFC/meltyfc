// MeltyFC — Typed Parameter Registry
// Every configurable value lives here. CLI enumerates this. Future GUI autogenerates from it.
// See spec §11, §11B.

#pragma once

#include <cstddef>
#include <cstdint>

namespace melty {

// ============================================================================
// Parameter types
// ============================================================================
enum class ParamType : uint8_t {
    UINT8,
    UINT16,
    UINT32,
    INT8,
    INT16,
    INT32,
    FLOAT,
    BOOL,
    ENUM, // Stored as uint8_t, display as string from enum table
};

// ============================================================================
// Parameter flags
// ============================================================================
enum class ParamFlags : uint8_t {
    NONE = 0,
    READONLY = (1 << 0),        // Cannot be set via CLI (computed/derived)
    NO_SAVE = (1 << 1),         // Not persisted (runtime-only)
    REQUIRES_REBOOT = (1 << 2), // Change takes effect after reboot
    FLOOR = (1 << 3),           // min is a hard floor (e.g., FAILSAFE_MS >= 500)
};

inline ParamFlags operator|(ParamFlags a, ParamFlags b) {
    return static_cast<ParamFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline bool operator&(ParamFlags a, ParamFlags b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

// ============================================================================
// Parameter definition (compile-time registry entry)
// ============================================================================
struct ParamDef {
    const char* name;        // CLI name (e.g., "WINDOW_HALF")
    const char* unit;        // Display unit (e.g., "deg", "mm", "ms", "%")
    const char* description; // Human-readable description
    ParamType type;
    ParamFlags flags;
    float min;
    float max;
    float defaultVal;
    uint16_t offset; // Byte offset into the config struct
};

// ============================================================================
// Config data struct — flat, memcpy-safe, versioned
// ============================================================================
struct __attribute__((packed)) ConfigData {
    // Schema version — increment on any layout change
    static constexpr uint16_t SCHEMA_VERSION = 1;
    uint16_t schemaVersion = SCHEMA_VERSION;

    // --- Bot geometry & drivetrain ---
    uint8_t numDriveMotors = 3;                         // 1–4
    float motorAngle[4] = {0.0f, 120.0f, 240.0f, 0.0f}; // degrees
    uint8_t spinDirection = 0;                          // 0=CW, 1=CCW
    uint16_t maxRpm = 3200;
    float rInner = 15.0f;           // mm
    float rOuter = 28.0f;           // mm
    float drEff = 13.0f;            // mm (derived, then cal-trimmed)
    float wheelDia = 40.0f;         // mm
    float wheelMountRadius = 85.0f; // mm
    float driveRatio = 5.0f;        // bell:wheel
    uint8_t motorPoles = 14;

    // --- Control & behavior ---
    float windowHalf = 30.0f;      // degrees
    float throttleSpinMax = 0.75f; // 0.0–1.0
    float throttleCap = 0.90f;     // 0.0–1.0
    float trimRateFine = 15.0f;    // deg/s at min deflection
    float trimRateMax = 360.0f;    // deg/s at full deflection
    float trimExpo = 0.3f;         // 0.0=linear, 1.0=max expo
    uint16_t orientDebounceMs = 150;
    float hitThreshG = 50.0f; // g threshold for hit detection
    uint16_t gyroBlankMs = 100;
    float omegaSlewMax = 200.0f; // rad/s per second
    float slipWarnPct = 25.0f;   // %
    uint16_t slipWarnMs = 300;
    uint16_t lowspeedSwitchRpm = 900;

    // --- RPM hold governor ---
    uint8_t rpmHoldEnabled = 0;      // 0=off, 1=on
    float rpmHoldKp = 0.002f;        // P gain
    float rpmHoldFeedforward = 0.5f; // base throttle fraction

    // --- Creep/tank mode ---
    uint16_t creepThresholdRpm = 200;
    uint16_t creepHysteresisRpm = 50;

    // --- LED ---
    uint16_t ledCount = 12;
    float ledArcWidth = 45.0f;                    // degrees
    uint8_t ledBeaconColorUp[3] = {0, 255, 0};    // RGB — green = upright
    uint8_t ledBeaconColorInv[3] = {255, 128, 0}; // RGB — orange = inverted
    uint8_t ledSafeColor[3] = {0, 0, 40};         // dim blue breathe
    uint8_t ledArmedColor[3] = {255, 0, 0};       // red
    uint8_t ledFailsafeColor[3] = {255, 0, 0};    // red strobe
    uint8_t ledLvcWarnColor[3] = {255, 255, 0};   // yellow overlay
    uint8_t ledLvcCritColor[3] = {255, 0, 0};     // red

    // --- RC channel map (CRSF channel indices, 0-based) ---
    uint8_t chTranslateX = 0; // Right stick horizontal (ail)
    uint8_t chTranslateY = 1; // Right stick vertical (ele)
    uint8_t chSpin = 2;       // Left stick vertical (thr)
    uint8_t chTrim = 3;       // Left stick horizontal (rud)
    uint8_t chArm = 4;        // AUX1 — arm switch
    uint8_t chResync = 5;     // AUX2 — heading re-sync
    uint8_t chCreepForce = 6; // AUX3 — force creep mode

    // --- Safety ---
    uint16_t failsafeMs = 500; // Hard floor: never < 500
    uint16_t watchdogMs = 200;

    // --- Low voltage ---
    float lvcWarnVolts = 3.3f; // Per-cell
    float lvcCritVolts = 3.0f; // Per-cell
    uint8_t cellCount = 0;     // 0 = auto-detect

    // --- Blackbox ---
    uint8_t blackboxEnabled = 1;
    uint16_t blackboxRateHz = 100; // Log rate

    // --- Translation deadband ---
    float translateDeadband = 0.05f; // 0.0–1.0 stick fraction

    // --- Resync ---
    float resyncMinDeflection = 0.3f; // Minimum stick deflection to accept re-sync
    uint16_t resyncAverageMs = 100;   // Average stick angle over this window

    // CRC for integrity check
    uint32_t crc32 = 0;
};

// ============================================================================
// Registry — compile-time array of all parameters
// ============================================================================

// Helper macro for defining params with offset
#define PARAM_OFFSET(field) (offsetof(ConfigData, field))

// The actual registry is defined in config_store.cpp
extern const ParamDef PARAM_REGISTRY[];
extern const size_t PARAM_REGISTRY_SIZE;

// Lookup by name — returns nullptr if not found
const ParamDef* findParam(const char* name);

// Get/set a param value in a ConfigData struct by ParamDef
float getParamFloat(const ConfigData& cfg, const ParamDef& def);
bool setParamFloat(ConfigData& cfg, const ParamDef& def, float value);

// Format a param value as string (for CLI output)
int formatParam(const ConfigData& cfg, const ParamDef& def, char* buf, size_t bufLen);

} // namespace melty
