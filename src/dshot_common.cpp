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

// Finding 6: Split throttle and command packers
uint16_t packThrottleFrame(uint16_t throttle, bool telemetryRequest) {
    // Clamp throttle to valid range: 0 (disarm) or 48-2047
    // Values 1-47 are DShot COMMANDS — NEVER emit as throttle
    if (throttle > 2047)
        throttle = 2047;
    if (throttle > 0 && throttle < DSHOT_MIN_THROTTLE)
        throttle = DSHOT_MIN_THROTTLE;

    uint16_t frame = (throttle << 5) | (telemetryRequest ? (1 << 4) : 0);
    frame |= computeCrc(frame);
    return frame;
}

uint16_t packCommandFrame(DisarmedOnlyToken /*token*/, uint16_t command, bool telemetryRequest) {
    // I-14: Token parameter enforces disarmed state at compile time
    // Command frames use values 1-47 — DO NOT clamp to 48
    if (command > 47)
        command = 47;

    uint16_t frame = (command << 5) | (telemetryRequest ? (1 << 4) : 0);
    frame |= computeCrc(frame);
    return frame;
}

uint16_t packThrottleBidir(uint16_t throttle, bool telemetryRequest) {
    if (throttle > 2047)
        throttle = 2047;
    if (throttle > 0 && throttle < DSHOT_MIN_THROTTLE)
        throttle = DSHOT_MIN_THROTTLE;

    uint16_t frame = (throttle << 5) | (telemetryRequest ? (1 << 4) : 0);
    frame |= computeCrc(frame) ^ 0x0F; // Inverted CRC for bidir
    return frame;
}

uint16_t packCommandBidir(DisarmedOnlyToken /*token*/, uint16_t command, bool telemetryRequest) {
    if (command > 47)
        command = 47;

    uint16_t frame = (command << 5) | (telemetryRequest ? (1 << 4) : 0);
    frame |= computeCrc(frame) ^ 0x0F;
    return frame;
}

// Legacy wrappers — map to the split versions
// packFrame/packFrameBidir are inlined in the header via the throttle versions

// ============================================================================
// Timer compare-buffer encoding
// ============================================================================

DshotTimingConfig calculateTiming(uint32_t timerClockHz, uint32_t dshotBitrateHz) {
    // Finding 2: bitrate MUST be in Hz (e.g., 300000 for DShot300).
    // Sanity check: valid DShot modes are 150k-1200k Hz.
    // Values < 100000 are almost certainly mode numbers (150, 300, etc.) — reject.
    if (dshotBitrateHz < 100000U || dshotBitrateHz > 1200000U) {
        // Return zero timing — caller should check and fail
        return {0, 0, 0};
    }

    uint32_t bitPeriod = timerClockHz / dshotBitrateHz;

    // Verify result fits in uint16_t (should be 70-560 for 84MHz clock)
    if (bitPeriod == 0 || bitPeriod > 65535U) {
        return {0, 0, 0};
    }

    return {
        static_cast<uint16_t>(bitPeriod),
        static_cast<uint16_t>(bitPeriod * 3 / 4), // 75% duty = bit 1
        static_cast<uint16_t>(bitPeriod * 3 / 8), // 37.5% duty = bit 0
    };
}

void encodeToCompareBuffer(uint16_t frame, const DshotTimingConfig& timing, uint16_t* buf) {
    // 16 data bits + 1 trailing zero (17th slot) to ensure line returns to idle.
    // Without the trailing zero, the last bit's level can persist — the ESC may
    // misinterpret the next frame's start.
    for (int i = 15; i >= 0; i--) {
        buf[15 - i] = (frame & (1 << i)) ? timing.bit1HighTicks : timing.bit0HighTicks;
    }
    buf[16] = 0; // Trailing idle — line returns low
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

    // F-03: Validate start bit (bit 20 must be 1)
    // Rejects shifted captures that nibble-validity alone would miss
    if (!(gcr21bit & (1u << 20))) {
        return 0xFFFF; // Missing start bit — shifted or corrupt frame
    }

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
    // Decoded frame after CRC validation: [period:12][crc:4]
    // Period encoding (EDT/bidir DShot):
    //   3-bit exponent + 9-bit mantissa: period_us = mantissa << exponent
    // NOT a literal 12-bit µs value.
    uint16_t raw12 = decodedFrame >> 4;
    uint16_t exponent = raw12 >> 9;     // top 3 bits
    uint16_t mantissa = raw12 & 0x01FF; // bottom 9 bits
    return mantissa << exponent;
}

bool isEdtExtendedFrame(uint16_t decodedFrame, bool edtNegotiated) {
    // R13-3: Must be gated on edtNegotiated. Without EDT negotiation,
    // ALL frames are eRPM — no discrimination applies.
    if (!edtNegotiated)
        return false;

    // R14-2: EDT frame identification per bird-sanctuary/extended-dshot-telemetry
    // spec, "Frame structure" section.
    //
    // 16-bit decoded frame layout: eeem mmmm mmmm cccc
    //   eee = 3-bit exponent (bits [15:13])
    //   m   = 9-bit mantissa (bits [12:4]), MSB at bit 12
    //   cccc = 4-bit checksum (bits [3:0])
    //
    // The PREFIX is bits [15:12] = [e2 e1 e0 m8] (exponent + MSB of mantissa).
    // Spec: "If [the prefix] is 0 OR the 8th bit is 1, it is a eRPM frame,
    //        the other ranges are EDT frames."
    //
    // EDT prefixes (even, non-zero):
    //   0b0010 (2)  = Temperature     0b1000 (8)  = Debug 1
    //   0b0100 (4)  = Voltage         0b1010 (10) = Debug 2
    //   0b0110 (6)  = Current         0b1100 (12) = Stress level
    //                                 0b1110 (14) = Status
    //
    // eRPM: prefix == 0 (stationary/low) OR odd prefix (m8 = 1).
    uint8_t prefix = (decodedFrame >> 12) & 0x0F;
    return prefix != 0 && (prefix & 1) == 0;
}

bool validateTelemetryCrc(uint16_t decodedFrame) {
    // Bidirectional DShot telemetry CRC is INVERTED (same as outgoing bidir frames)
    uint16_t period = decodedFrame >> 4;
    uint8_t expectedCrc = (~(period ^ (period >> 4) ^ (period >> 8))) & 0x0F;
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
