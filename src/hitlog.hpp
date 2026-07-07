// MeltyFC — Hit Logger
// PURE LOGIC — ring buffer of hit events with peak-G.
// Hit detection reuses the heading engine's detectHit().
// See spec §9C.

#pragma once

#include <cstddef>
#include <cstdint>

namespace melty {

static constexpr uint8_t HIT_LOG_SIZE = 64; // Ring buffer capacity

struct HitRecord {
    uint32_t timestampMs; // When the hit occurred
    float peakG;          // Peak g-force of the hit
    float omegaAtHit;     // Spin rate at time of hit (rad/s)
};

struct HitLogger {
    HitRecord entries[HIT_LOG_SIZE];
    uint8_t writeIdx;   // Next write position
    uint16_t totalHits; // Lifetime hit counter (wraps at 65535)
    float maxPeakG;     // Highest peak-G ever recorded
};

// Initialize the hit logger
void hitLogInit(HitLogger& log);

// Record a hit. Overwrites oldest entry when full.
void hitLogRecord(HitLogger& log, uint32_t timestampMs, float peakG, float omega);

// Get the most recent hit (nullptr if no hits)
const HitRecord* hitLogLatest(const HitLogger& log);

// Get hit count
uint16_t hitLogCount(const HitLogger& log);

// Get maximum peak-G recorded
float hitLogMaxG(const HitLogger& log);

// Format hit log summary for CLI `status` output
// Returns chars written.
int hitLogFormat(const HitLogger& log, char* buf, size_t bufLen);

} // namespace melty
