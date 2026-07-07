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
bool migrateConfig(const uint8_t* oldData, size_t oldSize, ConfigData& out);

// CRC32 for config integrity (excludes the crc32 field itself)
uint32_t computeConfigCrc(const ConfigData& cfg);

} // namespace melty
