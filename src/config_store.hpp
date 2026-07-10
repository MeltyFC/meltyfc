// MeltyFC — Config Store
// Implements IConfigStore. Internal flash, versioned schema, migration-safe.
// See spec §11, §11B.

#pragma once

#include "param_registry.h"
#include <cstddef>
#include <cstdint>

namespace melty {

// Migrate old config data to current schema.
// Returns true if migration successful (including same-version copy).
// Returns false if version incompatible (out gets defaults).
[[nodiscard]] bool migrateConfig(const uint8_t* oldData, size_t oldSize, ConfigData& out);

// CRC32 for config integrity (excludes the crc32 field itself)
[[nodiscard]] uint32_t computeConfigCrc(const ConfigData& cfg);

// ============================================================================
// Cross-param validation (Round 2 A1)
// Returns number of issues found. Issues are clamped/fixed in-place.
// Call at load + save + set.
// ============================================================================
struct ConfigValidationResult {
    uint8_t issueCount;
    bool spinMaxExceedsCap;  // THROTTLE_SPIN_MAX > THROTTLE_CAP
    bool innerGtOuter;       // R_INNER >= R_OUTER
    bool lvcWarnBelowCrit;   // LVC_WARN <= LVC_CRIT
    bool windowHalfTooLarge; // WINDOW_HALF > 90°
    bool channelCollision;   // Any two channel assignments collide
    bool numMotorsInvalid;   // NUM_DRIVE_MOTORS < 2 or > 4
    bool accelSaturation;    // R15-3: ω²r exceeds H3LIS ±400g (with margin)
    bool windowSamplingDead; // R15-4: windowHalf < 1.5× deg/sample → stochastic
};

// R15-5: Load-path per-field clamp — returns number of fields clamped.
// FLOOR-flagged params clamp UP; everything else clamps to [min,max].
// Call post-CRC, pre-cross-rules on loaded config.
[[nodiscard]] uint8_t clampConfigToRegistry(ConfigData& cfg);

// R17-2: Persistent flag — set after clampConfigToRegistry finds out-of-range values.
// Visible in CLI `status`, consumed by `save` notification.
struct ConfigLoadState {
    uint8_t clampedCount;    // Number of fields clamped on last load
    bool wasClampedOnLoad;   // True if any fields were clamped
};

[[nodiscard]] ConfigValidationResult validateConfig(ConfigData& cfg);

} // namespace melty
