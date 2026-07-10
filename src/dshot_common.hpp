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

// Minimum throttle value — values 1-47 are DShot COMMANDS, not throttle
static constexpr uint16_t DSHOT_MIN_THROTTLE = 48;

// Compare buffer size: 16 data bits + 1 trailing idle
static constexpr uint8_t DSHOT_COMPARE_BUF_SIZE = 17;

// Finding 6: Separate throttle and command packers.
// Throttle packer clamps 1-47→48 (safety).
// Command packer preserves command IDs (1-47 are valid commands).

// Pack a THROTTLE frame: 0 (disarm) or 48–2047. Values 1-47 clamped to 48.
[[nodiscard]] uint16_t packThrottleFrame(uint16_t throttle, bool telemetryRequest);

// C2/I-14: DisarmedOnlyToken — type-system guard for ESC command frames.
// Only the safety module can construct this token (when in DISARMED state).
// Callers cannot fake it — the constructor is private, safety.cpp is a friend.
struct DisarmedOnlyToken {
private:
    DisarmedOnlyToken() = default;
    friend struct SafetyModule; // Only safety.cpp can create these
#ifdef UNIT_TEST
public: // Tests can construct tokens directly
#endif
    // Production callers must obtain the token through the safety interface.
public:
    // Public copy/move for passing around once created
    DisarmedOnlyToken(const DisarmedOnlyToken&) = default;
};

// Pack a COMMAND frame: command ID (1-47) preserved, NOT clamped.
// I-14: Requires DisarmedOnlyToken — enforces disarmed state at compile time.
// The token parameter is unused at runtime but enforces that ONLY code with
// access to a DisarmedOnlyToken can call this function.
[[nodiscard]] uint16_t packCommandFrame(DisarmedOnlyToken token, uint16_t command, bool telemetryRequest);

// Pack a bidirectional THROTTLE frame: INVERTED CRC for bidir detection
[[nodiscard]] uint16_t packThrottleBidir(uint16_t throttle, bool telemetryRequest);

// Pack a bidirectional COMMAND frame: command ID + inverted CRC
[[nodiscard]] uint16_t packCommandBidir(DisarmedOnlyToken token, uint16_t command, bool telemetryRequest);

// Legacy aliases — DEPRECATED, use throttle/command-specific versions
inline uint16_t packFrame(uint16_t throttle, bool telemetryRequest) {
    return packThrottleFrame(throttle, telemetryRequest);
}
inline uint16_t packFrameBidir(uint16_t throttle, bool telemetryRequest) {
    return packThrottleBidir(throttle, telemetryRequest);
}

// Extract CRC from a packed frame
[[nodiscard]] uint8_t extractCrc(uint16_t frame);

// Compute DShot CRC (XOR of three nibbles)
[[nodiscard]] uint8_t computeCrc(uint16_t frameNoCrc);

// Validate a received frame's CRC
[[nodiscard]] bool validateCrc(uint16_t frame);

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
// Finding 2: dshotBitrateHz MUST be in Hz (300000 for DShot300, NOT 300).
// Returns zero timing if input is out of valid range (100k-1200k Hz).
[[nodiscard]] DshotTimingConfig calculateTiming(uint32_t timerClockHz, uint32_t dshotBitrateHz);

// Encode a 16-bit frame into a 17-element compare-buffer for DMA
// buf must have space for DSHOT_COMPARE_BUF_SIZE (17) entries
// Slot 17 is trailing zero to return line to idle
void encodeToCompareBuffer(uint16_t frame, const DshotTimingConfig& timing, uint16_t* buf);

// ============================================================================
// Bidirectional DShot: GCR (5-bit to 4-bit) decode
// ============================================================================

// GCR lookup table: 5-bit encoded → 4-bit decoded
// Invalid entries = 0xFF
extern const uint8_t GCR_DECODE_TABLE[32];

// Decode a 21-bit GCR-encoded telemetry frame to 16-bit value
// Returns decoded 16-bit value, or 0xFFFF on decode error
[[nodiscard]] uint16_t gcrDecode(uint32_t gcr21bit);

// Extract eRPM period from decoded telemetry frame
// EDT encoding: 3-bit exponent + 9-bit mantissa → period_us = mantissa << exponent
[[nodiscard]] uint16_t extractErpmPeriod(uint16_t decodedFrame);

// R14-2: Check if a decoded frame is EDT extended telemetry (not eRPM).
// Spec: bird-sanctuary/extended-dshot-telemetry, "Frame structure" section.
// Discrimination uses the PREFIX (bits [15:12] = exponent + MSB of mantissa):
//   EDT = even non-zero prefix; eRPM = prefix==0 or odd prefix.
// MUST be gated on edtNegotiated — without EDT, ALL frames are eRPM.
[[nodiscard]] bool isEdtExtendedFrame(uint16_t decodedFrame, bool edtNegotiated);

// Validate decoded telemetry CRC (INVERTED 4-bit XOR checksum)
[[nodiscard]] bool validateTelemetryCrc(uint16_t decodedFrame);

// Convert eRPM period to eRPM value
// period is in µs; eRPM = 60,000,000 / period
[[nodiscard]] uint32_t periodToErpm(uint16_t periodUs);

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

// ============================================================================
// A5: EDT handshake sequence
// To enable bidirectional DShot telemetry, the FC must send
// DSHOT_CMD_BIDIR_EDT_MODE_ON (cmd 13) at least 6 times while disarmed.
// This must happen before any throttle commands.
// ============================================================================
constexpr uint8_t EDT_ENABLE_REPEAT_COUNT = 6;

// B3/I-28: EDT enable through the token-typed command packer
// Takes DisarmedOnlyToken because EDT handshake MUST happen while disarmed
inline uint16_t packEdtEnableFrame(DisarmedOnlyToken token) {
    return packCommandBidir(token, DSHOT_CMD_BIDIR_EDT_MODE_ON, true);
}

// State machine for EDT handshake during arming
struct EdtHandshakeState {
    uint8_t sentCount; // How many EDT enable frames have been sent
    bool complete;     // Handshake done — safe to send throttle

    void reset() {
        sentCount = 0;
        complete = false;
    }

    // Call each loop iteration while disarmed and EDT is desired.
    // Returns the frame to send (0 when complete — switch to throttle frames).
    // B3: Takes DisarmedOnlyToken — EDT handshake is a disarmed-only operation
    uint16_t step(DisarmedOnlyToken token) {
        if (complete)
            return 0;
        sentCount++;
        if (sentCount >= EDT_ENABLE_REPEAT_COUNT) {
            complete = true;
        }
        return packEdtEnableFrame(token);
    }
};

} // namespace dshot
} // namespace melty
