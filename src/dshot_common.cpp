// MeltyFC — DShot Protocol Implementation
// Pure logic — frame packing, CRC, GCR decode.

#include "dshot_common.hpp"

namespace melty {
namespace dshot {

// ============================================================================
// CRC
// ============================================================================

uint8_t computeCrc(uint16_t frameNoCrc) {
    // DShot CRC = XOR of three nibbles of the 12-bit payload
    // payload = bits [15:4] of the full frame (throttle + telem bit)
    uint16_t payload = frameNoCrc >> 4;
    return (payload ^ (payload >> 4) ^ (payload >> 8)) & 0x0F;
}

uint8_t extractCrc(uint16_t frame) {
    return frame & 0x0F;
}

bool validateCrc(uint16_t frame) {
    uint16_t frameNoCrc = frame & 0xFFF0;
    return computeCrc(frameNoCrc) == extractCrc(frame);
}

// ============================================================================
// Frame packing
// ============================================================================

uint16_t packFrame(uint16_t throttle, bool telemetryRequest) {
    // Clamp throttle to 11 bits
    if (throttle > 2047)
        throttle = 2047;

    // Frame layout: [throttle:11][telem:1][crc:4]
    uint16_t frame = (throttle << 5) | (telemetryRequest ? (1 << 4) : 0);
    frame |= computeCrc(frame);
    return frame;
}

// ============================================================================
// Timer compare-buffer encoding
// ============================================================================

DshotTimingConfig calculateTiming(uint32_t timerClockHz, uint32_t dshotBitrate) {
    // DShot bit period in timer ticks
    uint32_t bitPeriod = timerClockHz / dshotBitrate;

    return {
        static_cast<uint16_t>(bitPeriod),
        static_cast<uint16_t>(bitPeriod * 3 / 4), // 75% duty = bit 1
        static_cast<uint16_t>(bitPeriod * 3 / 8), // 37.5% duty = bit 0
    };
}

void encodeToCompareBuffer(uint16_t frame, const DshotTimingConfig& timing, uint16_t* buf) {
    for (int i = 15; i >= 0; i--) {
        buf[15 - i] = (frame & (1 << i)) ? timing.bit1HighTicks : timing.bit0HighTicks;
    }
}

// ============================================================================
// GCR decode
// ============================================================================

// 5-bit GCR to 4-bit nibble lookup
// From Betaflight / BLHeli_S documentation
// 0xFF = invalid encoding
const uint8_t GCR_DECODE_TABLE[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, // 00000–00011
    0xFF, 0xFF, 0xFF, 0xFF, // 00100–00111
    0xFF, 0x09, 0x0A, 0x0B, // 01000–01011
    0xFF, 0x0D, 0x0E, 0x0F, // 01100–01111
    0xFF, 0xFF, 0x02, 0x03, // 10000–10011
    0xFF, 0x05, 0x06, 0x07, // 10100–10111
    0xFF, 0x00, 0x08, 0x01, // 11000–11011
    0xFF, 0x04, 0x0C, 0xFF, // 11100–11111
};

uint16_t gcrDecode(uint32_t gcr21bit) {
    // 21-bit GCR → 4 nibbles (each 5 GCR bits → 4 data bits) + 1 start bit
    // Layout: [start:1][gcr4:5][gcr3:5][gcr2:5][gcr1:5]
    // Total = 21 bits, produces 16 data bits

    uint16_t decoded = 0;

    for (int i = 0; i < 4; i++) {
        uint8_t gcr5 = (gcr21bit >> (i * 5)) & 0x1F;
        uint8_t nibble = GCR_DECODE_TABLE[gcr5];
        if (nibble == 0xFF) {
            return 0xFFFF; // Invalid GCR encoding
        }
        decoded |= (nibble << (i * 4));
    }

    return decoded;
}

uint16_t extractErpmPeriod(uint16_t decodedFrame) {
    // Decoded frame: [period:12][crc:4]
    // Period is in the upper 12 bits after CRC validation
    return decodedFrame >> 4;
}

bool validateTelemetryCrc(uint16_t decodedFrame) {
    // CRC is XOR of three nibbles of the 12-bit period
    uint16_t period = decodedFrame >> 4;
    uint8_t expectedCrc = (period ^ (period >> 4) ^ (period >> 8)) & 0x0F;
    uint8_t actualCrc = decodedFrame & 0x0F;
    return expectedCrc == actualCrc;
}

uint32_t periodToErpm(uint16_t periodUs) {
    if (periodUs == 0)
        return 0;
    // eRPM = 60,000,000 / period_us
    return 60000000UL / static_cast<uint32_t>(periodUs);
}

} // namespace dshot
} // namespace melty
