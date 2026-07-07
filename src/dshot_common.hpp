// MeltyFC — DShot Protocol: Frame Packing, CRC, GCR Decode
// Pure logic — no hardware dependencies. Testable natively.
// See spec §4, §9. This is the P2 spike core.
//
// DShot300 frame: 16 bits
//   [10:0] = throttle (0=disarm, 48–2047=throttle range)
//   [11]   = telemetry request bit
//   [15:12]= 4-bit CRC (XOR nibble checksum)
//
// Bidirectional DShot telemetry (ESC → FC):
//   ESC sends 21-bit GCR-encoded frame on inverted line
//   Decoded to 20 bits: [0:11]=eRPM period, [12:15]=CRC, [16:19]=unused
//   eRPM period → eRPM = 60e6 / period (in µs)

#pragma once

#include <cstdint>

namespace melty {
namespace dshot {

// ============================================================================
// DShot frame packing
// ============================================================================

// Pack a DShot frame: throttle (0–2047), telemetry bit, auto-CRC
// Returns the 16-bit frame ready for timer compare-buffer encoding
uint16_t packFrame(uint16_t throttle, bool telemetryRequest);

// Extract CRC from a packed frame
uint8_t extractCrc(uint16_t frame);

// Compute DShot CRC (XOR of three nibbles)
uint8_t computeCrc(uint16_t frameNoCrc);

// Validate a received frame's CRC
bool validateCrc(uint16_t frame);

// ============================================================================
// DShot timer compare-buffer encoding
// ============================================================================

// DShot300: bit period = 3.333µs at timer clock
// Bit 1 = high for 2.500µs (75%), low for 0.833µs
// Bit 0 = high for 1.250µs (37.5%), low for 2.083µs
//
// With timer running at e.g., 84MHz:
//   Bit period = 84MHz * 3.333µs = 280 ticks
//   Bit 1 high = 210 ticks (75%)
//   Bit 0 high = 105 ticks (37.5%)

struct DshotTimingConfig {
    uint16_t bitPeriodTicks; // Timer ARR for one bit
    uint16_t bit1HighTicks;  // Compare value for bit=1
    uint16_t bit0HighTicks;  // Compare value for bit=0
};

// Calculate timing for a given timer clock frequency and DShot bitrate
DshotTimingConfig calculateTiming(uint32_t timerClockHz, uint32_t dshotBitrate);

// Encode a 16-bit frame into a 16-element compare-buffer for DMA
// buf must have space for 16 entries (+ optional 1 reset entry)
void encodeToCompareBuffer(uint16_t frame, const DshotTimingConfig& timing, uint16_t* buf);

// ============================================================================
// Bidirectional DShot: GCR (5-bit to 4-bit) decode
// ============================================================================

// GCR lookup table: 5-bit encoded → 4-bit decoded
// Invalid entries = 0xFF
extern const uint8_t GCR_DECODE_TABLE[32];

// Decode a 21-bit GCR-encoded telemetry frame to 16-bit value
// Returns decoded 16-bit value, or 0xFFFF on decode error
uint16_t gcrDecode(uint32_t gcr21bit);

// Extract eRPM period from decoded telemetry frame
// Returns period in µs (12-bit), or 0 on invalid
uint16_t extractErpmPeriod(uint16_t decodedFrame);

// Validate decoded telemetry CRC (4-bit XOR checksum)
bool validateTelemetryCrc(uint16_t decodedFrame);

// Convert eRPM period to eRPM value
// period is in µs; eRPM = 60,000,000 / period
uint32_t periodToErpm(uint16_t periodUs);

// ============================================================================
// DShot special commands (sent as throttle values 1–47)
// ============================================================================

enum DshotCommand : uint16_t {
    DSHOT_CMD_BEEP1 = 1,
    DSHOT_CMD_BEEP2 = 2,
    DSHOT_CMD_BEEP3 = 3,
    DSHOT_CMD_BEEP4 = 4,
    DSHOT_CMD_BEEP5 = 5,
    DSHOT_CMD_INFO_REQUEST = 6,
    DSHOT_CMD_SPIN_DIRECTION_1 = 7,
    DSHOT_CMD_SPIN_DIRECTION_2 = 8,
    DSHOT_CMD_3D_MODE_OFF = 9,
    DSHOT_CMD_3D_MODE_ON = 10,
    DSHOT_CMD_SAVE_SETTINGS = 12,
    DSHOT_CMD_BIDIR_EDT_MODE_ON = 13,
    DSHOT_CMD_BIDIR_EDT_MODE_OFF = 14,
};

} // namespace dshot
} // namespace melty
