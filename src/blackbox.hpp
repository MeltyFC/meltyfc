// MeltyFC — Blackbox-Lite
// PURE LOGIC — ring buffer format and sector management.
// Actual SPI flash writes are hardware-specific (via IFlashStorage).
// See spec §9C: ring log of omega/hits/slip/vbat/orientation to 8MB flash.

#pragma once

#include <cstdint>
#include <cstddef>

namespace melty {

// ============================================================================
// Blackbox record — fixed-size for ring buffer alignment
// ============================================================================
struct __attribute__((packed)) BlackboxRecord {
    uint32_t timestampMs;       // Timestamp
    float    omega;             // Spin rate (rad/s)
    float    phase;             // Bot phase (rad)
    float    packVoltage;       // Pack voltage (V)
    float    slipPct;           // Slip percentage (0-100)
    uint8_t  orientation;       // 0=upright, 1=inverted
    uint8_t  hitDetected;       // Hit on this sample
    uint8_t  armState;          // 0=disarmed, 1=armed, 2=failsafe
    uint8_t  reserved;          // Alignment padding
    // Total: 4+4+4+4+4+1+1+1+1 = 24 bytes
};

static_assert(sizeof(BlackboxRecord) == 24, "BlackboxRecord must be 24 bytes");

// ============================================================================
// Ring buffer state (pure logic — tracks write position)
// ============================================================================
struct BlackboxState {
    uint32_t writeOffset;       // Current write offset in flash (bytes)
    uint32_t flashSize;         // Total flash available (bytes)
    uint32_t sectorSize;        // Flash sector size (bytes, e.g., 4096)
    uint32_t recordCount;       // Total records written (lifetime, wraps)
    bool     wrapped;           // Has the ring wrapped around?
};

// ============================================================================
// Pure logic functions
// ============================================================================

// Initialize blackbox state
void blackboxInit(BlackboxState& state, uint32_t flashSize, uint32_t sectorSize);

// Compute the next write offset, handling wraparound.
// Returns the offset to write the next record.
// Advances the write pointer.
uint32_t blackboxNextOffset(BlackboxState& state);

// Check if a sector erase is needed before writing.
// Returns true + sets eraseAddr if the write would cross a sector boundary.
bool blackboxNeedsErase(const BlackboxState& state, uint32_t* eraseAddr);

// How many records can fit in the flash?
uint32_t blackboxCapacity(const BlackboxState& state);

// How many records are currently stored?
uint32_t blackboxStored(const BlackboxState& state);

// Compute the read offset for the Nth most recent record (0 = most recent).
// Returns false if N >= stored records.
bool blackboxReadOffset(const BlackboxState& state, uint32_t n, uint32_t* offset);

// ============================================================================
// CLI dump format
// ============================================================================

// Format a blackbox record as CSV for CLI dump.
// Returns chars written.
int blackboxFormatRecord(const BlackboxRecord& rec, char* buf, size_t bufLen);

// Format CSV header
int blackboxFormatHeader(char* buf, size_t bufLen);

} // namespace melty
