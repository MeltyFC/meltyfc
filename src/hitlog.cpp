// MeltyFC — Hit Logger Implementation

#include "hitlog.hpp"
#include <cstring>
#include <cstdio>

namespace melty {

void hitLogInit(HitLogger& log) {
    memset(&log, 0, sizeof(log));
}

void hitLogRecord(HitLogger& log, uint32_t timestampMs, float peakG, float omega) {
    HitRecord& entry = log.entries[log.writeIdx];
    entry.timestampMs = timestampMs;
    entry.peakG = peakG;
    entry.omegaAtHit = omega;

    log.writeIdx = (log.writeIdx + 1) % HIT_LOG_SIZE;
    if (log.totalHits < 65535) log.totalHits++;
    if (peakG > log.maxPeakG) log.maxPeakG = peakG;
}

const HitRecord* hitLogLatest(const HitLogger& log) {
    if (log.totalHits == 0) return nullptr;
    uint8_t idx = (log.writeIdx == 0) ? HIT_LOG_SIZE - 1 : log.writeIdx - 1;
    return &log.entries[idx];
}

uint16_t hitLogCount(const HitLogger& log) {
    return log.totalHits;
}

float hitLogMaxG(const HitLogger& log) {
    return log.maxPeakG;
}

int hitLogFormat(const HitLogger& log, char* buf, size_t bufLen) {
    int written = 0;
    int n;

    n = snprintf(buf + written, bufLen - written,
        "Hits: %u  MaxG: %.1f\r\n", log.totalHits, (double)log.maxPeakG);
    if (n > 0) written += n;

    const HitRecord* latest = hitLogLatest(log);
    if (latest) {
        float rpm = latest->omegaAtHit * 60.0f / 6.2831853f;
        n = snprintf(buf + written, bufLen - written,
            "Last: %.1fg @ %.0f RPM (t=%ums)\r\n",
            (double)latest->peakG, (double)rpm, latest->timestampMs);
        if (n > 0) written += n;
    }

    return written;
}

} // namespace melty
