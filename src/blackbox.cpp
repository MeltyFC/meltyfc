#include <cinttypes>
// MeltyFC — Blackbox-Lite Implementation
// DI-09/10/11/12 fixes: usable size precompute, sector-start erase,
// no cross-sector records, record magic for validity.

#include "blackbox.hpp"
#include <cstdio>

namespace melty {

void blackboxInit(BlackboxState& state, uint32_t flashSize, uint32_t sectorSize) {
    state.writeOffset = 0;
    state.flashSize = flashSize;
    state.sectorSize = sectorSize;
    state.recordCount = 0;
    state.wrapped = false;
    // DI-09: Precompute usable size — exact multiple of record size
    state.usableSize = (flashSize / sizeof(BlackboxRecord)) * sizeof(BlackboxRecord);
}

uint32_t blackboxNextOffset(BlackboxState& state) {
    // DI-09: Wrap when we can't fit another record
    if (state.writeOffset >= state.usableSize) {
        state.writeOffset = 0;
        state.wrapped = true;
    }

    // DI-11: Skip to next sector if record would straddle a sector boundary
    if (state.sectorSize > 0) {
        uint32_t sectorStart = (state.writeOffset / state.sectorSize) * state.sectorSize;
        uint32_t sectorEnd = sectorStart + state.sectorSize;
        if (state.writeOffset + sizeof(BlackboxRecord) > sectorEnd) {
            // Record would cross sector boundary — skip to next sector
            state.writeOffset = sectorEnd;
            // Check wrap again after skip
            if (state.writeOffset + sizeof(BlackboxRecord) > state.usableSize) {
                state.writeOffset = 0;
                state.wrapped = true;
            }
        }
    }

    uint32_t offset = state.writeOffset;
    state.writeOffset += sizeof(BlackboxRecord);
    state.recordCount++;
    return offset;
}

bool blackboxNeedsErase(const BlackboxState& state, uint32_t* eraseAddr) {
    if (state.sectorSize == 0)
        return false;

    // DI-10: Erase when write offset is at the start of ANY sector, not just 0
    if (state.writeOffset % state.sectorSize == 0) {
        if (eraseAddr)
            *eraseAddr = state.writeOffset;
        return true;
    }
    return false;
}

uint32_t blackboxCapacity(const BlackboxState& state) {
    return state.usableSize / sizeof(BlackboxRecord);
}

uint32_t blackboxStored(const BlackboxState& state) {
    uint32_t cap = blackboxCapacity(state);
    if (state.wrapped)
        return cap;
    return state.writeOffset / sizeof(BlackboxRecord);
}

bool blackboxReadOffset(const BlackboxState& state, uint32_t n, uint32_t* offset) {
    uint32_t stored = blackboxStored(state);
    if (n >= stored)
        return false;

    int64_t readOff = static_cast<int64_t>(state.writeOffset) -
                      static_cast<int64_t>((n + 1) * sizeof(BlackboxRecord));

    if (readOff < 0) {
        if (!state.wrapped)
            return false;
        readOff += state.usableSize; // DI-09: Use usableSize, not flashSize
    }

    *offset = static_cast<uint32_t>(readOff);
    return true;
}

int blackboxFormatHeader(char* buf, size_t bufLen) {
    return snprintf(buf, bufLen,
                    "timestamp_ms,omega_rad_s,phase_rad,pack_v,slip_pct,"
                    "orientation,hit,arm_state\r\n");
}

int blackboxFormatRecord(const BlackboxRecord& rec, char* buf, size_t bufLen) {
    // DI-12: Check record magic before formatting
    if (rec.magic != BLACKBOX_RECORD_MAGIC) {
        return snprintf(buf, bufLen, "# invalid record (magic=0x%04X)\r\n", rec.magic);
    }
    return snprintf(buf, bufLen, "%" PRIu32 ",%.2f,%.4f,%.2f,%.1f,%u,%u,%u\r\n", rec.timestampMs,
                    (double)rec.omega, (double)rec.phase, (double)rec.packVoltage,
                    (double)rec.slipPct, rec.orientation, rec.hitDetected, rec.armState);
}

} // namespace melty
