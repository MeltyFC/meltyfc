// MeltyFC — A/B Config Flash Storage Implementation

#include "config_flash.hpp"
#include "config_store.hpp"

namespace melty {

SlotStatus validateSlot(const ConfigSlotHeader& header, const ConfigData& data) {
    SlotStatus status = {false, 0};

    // Check magic
    if (header.magic != CONFIG_SLOT_MAGIC)
        return status;

    // Check CRC
    uint32_t computed = computeConfigCrc(data);
    if (computed != data.crc32)
        return status;

    // Check schema version
    if (data.schemaVersion != ConfigData::SCHEMA_VERSION)
        return status;

    status.valid = true;
    status.sequence = header.sequence;
    return status;
}

int pickActiveSlot(const SlotStatus& slot0, const SlotStatus& slot1) {
    if (!slot0.valid && !slot1.valid)
        return -1; // Both invalid — use defaults

    if (slot0.valid && !slot1.valid)
        return 0;
    if (!slot0.valid && slot1.valid)
        return 1;

    // Both valid — pick higher sequence, handling wrap-around
    // Sequence wraps 0-255. The "newer" slot is the one where
    // (a - b) mod 256 < 128 (i.e., a is ahead of b by less than half)
    uint8_t diff = slot0.sequence - slot1.sequence;
    if (diff == 0)
        return 0; // Same sequence — shouldn't happen, pick 0
    return (diff < 128) ? 0 : 1;
}

int pickWriteSlot(const SlotStatus& slot0, const SlotStatus& slot1) {
    if (!slot0.valid && !slot1.valid)
        return 0; // Both invalid — write to 0

    if (!slot0.valid)
        return 0; // Slot 0 invalid — overwrite it
    if (!slot1.valid)
        return 1; // Slot 1 invalid — overwrite it

    // Both valid — overwrite the OLDER one (lower sequence)
    return (pickActiveSlot(slot0, slot1) == 0) ? 1 : 0;
}

uint8_t nextSequence(uint8_t current) {
    return static_cast<uint8_t>(current + 1); // Wraps at 255→0
}

ConfigSlotHeader buildSlotHeader(uint8_t sequence) {
    ConfigSlotHeader header;
    header.magic = CONFIG_SLOT_MAGIC;
    header.sequence = sequence;
    header.reserved[0] = 0;
    header.reserved[1] = 0;
    header.reserved[2] = 0;
    return header;
}

} // namespace melty
