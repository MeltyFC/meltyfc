// MeltyFC — CRSF Protocol Parser + Telemetry Frame Builder
// PURE LOGIC — no UART hardware. Takes raw bytes, outputs parsed channels.
// See spec §8, §9. CRSF protocol spec: frame 0x16 channels, 0x14 link stats,
// 0x21 flight mode. CRC8 DVB-S2 polynomial 0xD5.

#pragma once

#include <cstdint>
#include <cstddef>

namespace melty {

// ============================================================================
// CRSF constants
// ============================================================================
static constexpr uint8_t  CRSF_SYNC_BYTE       = 0xC8;
static constexpr uint8_t  CRSF_SYNC_BYTE_ALT   = 0xEE; // EdgeTX agent address
static constexpr size_t   CRSF_MAX_FRAME_SIZE  = 64;
static constexpr size_t   CRSF_MAX_CHANNELS    = 16;
static constexpr uint16_t CRSF_CHANNEL_MID     = 992;  // Center value
static constexpr uint16_t CRSF_CHANNEL_MIN     = 172;  // Minimum
static constexpr uint16_t CRSF_CHANNEL_MAX     = 1811; // Maximum

// Frame types
static constexpr uint8_t CRSF_FRAMETYPE_RC_CHANNELS    = 0x16;
static constexpr uint8_t CRSF_FRAMETYPE_LINK_STATS     = 0x14;
static constexpr uint8_t CRSF_FRAMETYPE_FLIGHT_MODE    = 0x21;
static constexpr uint8_t CRSF_FRAMETYPE_BATTERY        = 0x08;
static constexpr uint8_t CRSF_FRAMETYPE_DEVICE_INFO    = 0x29;

// Device addresses
static constexpr uint8_t CRSF_ADDRESS_FC              = 0xC8;
static constexpr uint8_t CRSF_ADDRESS_TX              = 0xEA;
static constexpr uint8_t CRSF_ADDRESS_RX              = 0xEC;

// ============================================================================
// Parsed channel data
// ============================================================================
struct CrsfChannels {
    uint16_t ch[CRSF_MAX_CHANNELS];  // Raw 11-bit values (172–1811)
    bool     valid;                   // CRC passed
    uint32_t timestamp;               // When parsed (caller sets)
};

// ============================================================================
// Link statistics
// ============================================================================
struct CrsfLinkStats {
    uint8_t  uplinkRssi1;
    uint8_t  uplinkRssi2;
    uint8_t  uplinkLinkQuality;
    int8_t   uplinkSnr;
    uint8_t  activeAntenna;
    uint8_t  rfMode;
    uint8_t  uplinkTxPower;
    uint8_t  downlinkRssi;
    uint8_t  downlinkLinkQuality;
    int8_t   downlinkSnr;
    bool     valid;
};

// ============================================================================
// Parser state machine
// ============================================================================
enum class CrsfParserState : uint8_t {
    SYNC,       // Waiting for sync byte
    LENGTH,     // Reading frame length
    PAYLOAD,    // Reading frame payload + CRC
};

struct CrsfParser {
    CrsfParserState state;
    uint8_t  frameBuf[CRSF_MAX_FRAME_SIZE];
    uint8_t  frameLen;      // Expected frame length (from header)
    uint8_t  frameIdx;      // Current position in buffer
};

// ============================================================================
// CRC8 DVB-S2 (polynomial 0xD5)
// ============================================================================
uint8_t crc8DvbS2(uint8_t crc, uint8_t data);
uint8_t crc8DvbS2Block(const uint8_t* data, size_t len);

// ============================================================================
// Parser functions
// ============================================================================

// Initialize parser state
void crsfParserInit(CrsfParser& parser);

// Feed one byte to the parser.
// Returns true if a complete valid frame is now in parser.frameBuf.
bool crsfParserFeed(CrsfParser& parser, uint8_t byte);

// Decode RC channels from a completed frame (type 0x16).
// Returns false if frame type doesn't match or CRC fails.
bool crsfDecodeChannels(const uint8_t* frame, uint8_t frameLen, CrsfChannels& out);

// Decode link stats from a completed frame (type 0x14).
bool crsfDecodeLinkStats(const uint8_t* frame, uint8_t frameLen, CrsfLinkStats& out);

// ============================================================================
// Channel utility — convert raw 11-bit to normalized float
// ============================================================================

// Convert raw channel value to -1.0..+1.0 (stick axis)
float crsfChannelToFloat(uint16_t raw);

// Convert raw channel value to 0.0..1.0 (throttle)
float crsfChannelToThrottle(uint16_t raw);

// Convert raw channel value to bool (switch, threshold at mid)
bool crsfChannelToBool(uint16_t raw);

// ============================================================================
// Telemetry frame builders (output bytes to send on UART TX)
// ============================================================================

// Build flight mode text frame (0x21) — e.g., "UPRT", "INVT", "SAFE", "CFG"
// Returns bytes written to outBuf.
size_t crsfBuildFlightMode(uint8_t* outBuf, size_t bufLen, const char* modeText);

// Build battery sensor frame (0x08) — voltage, current, capacity, remaining
size_t crsfBuildBattery(uint8_t* outBuf, size_t bufLen,
                        float voltage, float current,
                        uint32_t capacityMah, uint8_t remainingPct);

} // namespace melty
