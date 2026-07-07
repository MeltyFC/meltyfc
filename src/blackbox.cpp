// MeltyFC — Blackbox-Lite Implementation
// PURE LOGIC — ring buffer management. Flash I/O via IFlashStorage.

#include "blackbox.hpp"
#include <cstdio>

namespace melty {

void blackboxInit(BlackboxState& state, uint32_t flashSize, uint32_t sectorSize) {
    state.writeOffset = 0;
    state.flashSize = flashSize;
    state.sectorSize = sectorSize;
    state.recordCount = 0;
    state.wrapped = false;
}

uint32_t blackboxNextOffset(BlackboxState& state) {
    uint32_t offset = state.writeOffset;

    state.writeOffset += sizeof(BlackboxRecord);
    if (state.writeOffset >= state.flashSize) {
        state.writeOffset = 0;
        state.wrapped = true;
    }

    state.recordCount++;
    return offset;
}

bool blackboxNeedsErase(const BlackboxState& state, uint32_t* eraseAddr) {
    // Erase needed when write offset is at the start of a new sector
    if (state.sectorSize == 0)
        return false;

    uint32_t nextWrite = state.writeOffset + sizeof(BlackboxRecord);
    uint32_t currentSector = state.writeOffset / state.sectorSize;
    uint32_t nextSector = nextWrite / state.sectorSize;

    if (nextSector != currentSector || state.writeOffset == 0) {
        if (eraseAddr)
            *eraseAddr = nextSector * state.sectorSize;
        return true;
    }
    return false;
}

uint32_t blackboxCapacity(const BlackboxState& state) {
    return state.flashSize / sizeof(BlackboxRecord);
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

    // Most recent is at (writeOffset - sizeof(BlackboxRecord))
    // N=0 → most recent, N=1 → second most recent, etc.
    int64_t readOff = static_cast<int64_t>(state.writeOffset) -
                      static_cast<int64_t>((n + 1) * sizeof(BlackboxRecord));

    if (readOff < 0) {
        if (!state.wrapped)
            return false;
        readOff += state.flashSize;
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
    return snprintf(buf, bufLen, "%u,%.2f,%.4f,%.2f,%.1f,%u,%u,%u\r\n", rec.timestampMs,
                    (double)rec.omega, (double)rec.phase, (double)rec.packVoltage,
                    (double)rec.slipPct, rec.orientation, rec.hitDetected, rec.armState);
}

} // namespace melty
