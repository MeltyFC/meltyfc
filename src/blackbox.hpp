// MeltyFC — Blackbox-Lite
// DI-09/10/11/12 fixes: usable size, sector-start erase, no cross-sector records, record magic.
// C8: Tier-gated — compiled out on TIER_MIN targets (256KB flash)

#pragma once

#include "feature_tiers.h"

#if MELTYFC_HAS_BLACKBOX

#include <cstddef>
#include <cstdint>

namespace melty {

// DI-12: Record magic for validity detection
constexpr uint16_t BLACKBOX_RECORD_MAGIC = 0xBB01;

// ============================================================================
// Blackbox record — fixed-size for ring buffer alignment
// DI-12: magic field added for torn-write detection
// ============================================================================
struct __attribute__((packed)) BlackboxRecord {
    uint16_t magic;       // BLACKBOX_RECORD_MAGIC (DI-12)
    uint16_t reserved;    // Alignment
    uint32_t timestampMs; // Timestamp
    float omega;          // Spin rate (rad/s)
    float phase;          // Bot phase (rad)
    float packVoltage;    // Pack voltage (V)
    float slipPct;        // Slip percentage (0-100)
    uint8_t orientation;  // 0=upright, 1=inverted
    uint8_t hitDetected;  // Hit on this sample
    uint8_t armState;     // 0=disarmed, 1=armed, 2=failsafe
    uint8_t padding;      // Alignment
    // Total: 2+2+4+4+4+4+4+1+1+1+1 = 28 bytes
};

static_assert(sizeof(BlackboxRecord) == 28, "BlackboxRecord must be 28 bytes");

// ============================================================================
// Ring buffer state
// ============================================================================
struct BlackboxState {
    uint32_t writeOffset; // Current write offset in flash (bytes)
    uint32_t flashSize;   // Total flash available (bytes)
    uint32_t usableSize;  // DI-09: Exact multiple of record size (precomputed)
    uint32_t sectorSize;  // Flash sector size (bytes, e.g., 4096)
    uint32_t recordCount; // Total records written (lifetime, wraps)
    bool wrapped;         // Has the ring wrapped around?
};

// ============================================================================
// Pure logic functions
// ============================================================================

void blackboxInit(BlackboxState& state, uint32_t flashSize, uint32_t sectorSize);
uint32_t blackboxNextOffset(BlackboxState& state);
bool blackboxNeedsErase(const BlackboxState& state, uint32_t* eraseAddr);
uint32_t blackboxCapacity(const BlackboxState& state);
uint32_t blackboxStored(const BlackboxState& state);
bool blackboxReadOffset(const BlackboxState& state, uint32_t n, uint32_t* offset);

int blackboxFormatRecord(const BlackboxRecord& rec, char* buf, size_t bufLen);
int blackboxFormatHeader(char* buf, size_t bufLen);

} // namespace melty

#endif // MELTYFC_HAS_BLACKBOX
