// MeltyFC — A/B Config Flash Storage (S1, R3-1)
// PURE LOGIC — A/B slot selection, CRC32 verification, sequence byte.
// The actual flash read/write goes through IFlashStorage.
//
// Design: two slots on SPI flash, each with:
//   [ConfigSlot header (8 bytes)] [ConfigData payload]
// On save: write to the OLDER slot, read-back-verify, then it becomes current.
// On load: read both slots, pick the one with valid CRC + higher sequence.
// Power loss mid-write: torn slot fails CRC → older slot loads. Rocket-clean.

#pragma once

#include "param_registry.h"
#include <cstdint>

namespace melty {

struct ConfigSlotHeader {
    uint32_t magic;   // 0x4D454C54 ("MELT")
    uint8_t sequence; // Monotonic counter (wraps at 255)
    uint8_t reserved[3];
};

constexpr uint32_t CONFIG_SLOT_MAGIC = 0x4D454C54; // "MELT"
constexpr size_t CONFIG_SLOT_SIZE = sizeof(ConfigSlotHeader) + sizeof(ConfigData);

struct SlotStatus {
    bool valid;       // CRC matches + magic correct
    uint8_t sequence; // Sequence byte (0-255)
};

// Check if a slot header + data is valid
SlotStatus validateSlot(const ConfigSlotHeader& header, const ConfigData& data);

// Determine which slot to LOAD from (valid + higher sequence wins)
// Returns 0 or 1. Returns -1 if both invalid (use defaults).
int pickActiveSlot(const SlotStatus& slot0, const SlotStatus& slot1);

// Determine which slot to WRITE to (the older/invalid one)
int pickWriteSlot(const SlotStatus& slot0, const SlotStatus& slot1);

// Compute next sequence byte
uint8_t nextSequence(uint8_t current);

// Build a slot header for writing
ConfigSlotHeader buildSlotHeader(uint8_t sequence);

} // namespace melty
