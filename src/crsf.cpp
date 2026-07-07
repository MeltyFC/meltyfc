// MeltyFC — CRSF Protocol Implementation
// PURE LOGIC — parser + telemetry frame builder. No UART hardware.

#include "crsf.hpp"
#include <cstring>
#include <cmath>

namespace melty {

// ============================================================================
// CRC8 DVB-S2 (polynomial 0xD5)
// ============================================================================
uint8_t crc8DvbS2(uint8_t crc, uint8_t data) {
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) crc = (crc << 1) ^ 0xD5;
        else crc <<= 1;
    }
    return crc;
}

uint8_t crc8DvbS2Block(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = crc8DvbS2(crc, data[i]);
    }
    return crc;
}

// ============================================================================
// Parser
// ============================================================================
void crsfParserInit(CrsfParser& parser) {
    parser.state = CrsfParserState::SYNC;
    parser.frameLen = 0;
    parser.frameIdx = 0;
}

bool crsfParserFeed(CrsfParser& parser, uint8_t byte) {
    switch (parser.state) {
        case CrsfParserState::SYNC:
            if (byte == CRSF_SYNC_BYTE || byte == CRSF_SYNC_BYTE_ALT) {
                parser.frameBuf[0] = byte;
                parser.frameIdx = 1;
                parser.state = CrsfParserState::LENGTH;
            }
            return false;

        case CrsfParserState::LENGTH:
            if (byte < 2 || byte > CRSF_MAX_FRAME_SIZE - 2) {
                parser.state = CrsfParserState::SYNC;
                return false;
            }
            parser.frameBuf[1] = byte;
            parser.frameLen = byte;
            parser.frameIdx = 2;
            parser.state = CrsfParserState::PAYLOAD;
            return false;

        case CrsfParserState::PAYLOAD:
            parser.frameBuf[parser.frameIdx++] = byte;
            if (parser.frameIdx >= parser.frameLen + 2u) {
                uint8_t crc = crc8DvbS2Block(&parser.frameBuf[2], parser.frameLen - 1);
                bool valid = (crc == parser.frameBuf[parser.frameIdx - 1]);
                parser.state = CrsfParserState::SYNC;
                return valid;
            }
            return false;
    }
    return false;
}

// ============================================================================
// Decode RC channels (frame type 0x16)
// 16x 11-bit values packed into 22 bytes
// ============================================================================
bool crsfDecodeChannels(const uint8_t* frame, uint8_t frameLen, CrsfChannels& out) {
    if (frameLen < 24 || frame[2] != CRSF_FRAMETYPE_RC_CHANNELS) return false;

    const uint8_t* p = &frame[3];

    out.ch[0]  = ((p[0]       | p[1]  << 8) & 0x07FF);
    out.ch[1]  = ((p[1]  >> 3 | p[2]  << 5) & 0x07FF);
    out.ch[2]  = ((p[2]  >> 6 | p[3]  << 2 | p[4] << 10) & 0x07FF);
    out.ch[3]  = ((p[4]  >> 1 | p[5]  << 7) & 0x07FF);
    out.ch[4]  = ((p[5]  >> 4 | p[6]  << 4) & 0x07FF);
    out.ch[5]  = ((p[6]  >> 7 | p[7]  << 1 | p[8] << 9) & 0x07FF);
    out.ch[6]  = ((p[8]  >> 2 | p[9]  << 6) & 0x07FF);
    out.ch[7]  = ((p[9]  >> 5 | p[10] << 3) & 0x07FF);
    out.ch[8]  = ((p[11]      | p[12] << 8) & 0x07FF);
    out.ch[9]  = ((p[12] >> 3 | p[13] << 5) & 0x07FF);
    out.ch[10] = ((p[13] >> 6 | p[14] << 2 | p[15] << 10) & 0x07FF);
    out.ch[11] = ((p[15] >> 1 | p[16] << 7) & 0x07FF);
    out.ch[12] = ((p[16] >> 4 | p[17] << 4) & 0x07FF);
    out.ch[13] = ((p[17] >> 7 | p[18] << 1 | p[19] << 9) & 0x07FF);
    out.ch[14] = ((p[19] >> 2 | p[20] << 6) & 0x07FF);
    out.ch[15] = ((p[20] >> 5 | p[21] << 3) & 0x07FF);

    out.valid = true;
    return true;
}

// ============================================================================
// Decode link statistics (frame type 0x14)
// ============================================================================
bool crsfDecodeLinkStats(const uint8_t* frame, uint8_t frameLen, CrsfLinkStats& out) {
    if (frameLen < 12 || frame[2] != CRSF_FRAMETYPE_LINK_STATS) return false;

    const uint8_t* p = &frame[3];
    out.uplinkRssi1         = p[0];
    out.uplinkRssi2         = p[1];
    out.uplinkLinkQuality   = p[2];
    out.uplinkSnr           = static_cast<int8_t>(p[3]);
    out.activeAntenna       = p[4];
    out.rfMode              = p[5];
    out.uplinkTxPower       = p[6];
    out.downlinkRssi        = p[7];
    out.downlinkLinkQuality = p[8];
    out.downlinkSnr         = static_cast<int8_t>(p[9]);
    out.valid = true;
    return true;
}

// ============================================================================
// Channel conversions
// ============================================================================
float crsfChannelToFloat(uint16_t raw) {
    return (static_cast<float>(raw) - static_cast<float>(CRSF_CHANNEL_MID))
         / (static_cast<float>(CRSF_CHANNEL_MAX - CRSF_CHANNEL_MIN) / 2.0f);
}

float crsfChannelToThrottle(uint16_t raw) {
    float norm = (static_cast<float>(raw) - static_cast<float>(CRSF_CHANNEL_MIN))
               / static_cast<float>(CRSF_CHANNEL_MAX - CRSF_CHANNEL_MIN);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return norm;
}

bool crsfChannelToBool(uint16_t raw) {
    return raw > CRSF_CHANNEL_MID;
}

// ============================================================================
// Telemetry: Flight Mode frame (0x21)
// ============================================================================
size_t crsfBuildFlightMode(uint8_t* outBuf, size_t bufLen, const char* modeText) {
    size_t textLen = strlen(modeText);
    if (textLen > 14) textLen = 14;

    // sync + length + type + dest + origin + text + null + crc
    size_t payloadLen = 1 + 1 + 1 + textLen + 1;  // type + dest + origin + text + null
    size_t frameSize = 2 + payloadLen + 1;          // sync + length + payload + crc
    if (bufLen < frameSize) return 0;

    size_t idx = 0;
    outBuf[idx++] = CRSF_ADDRESS_FC;
    outBuf[idx++] = static_cast<uint8_t>(payloadLen + 1);  // length includes CRC

    outBuf[idx++] = CRSF_FRAMETYPE_FLIGHT_MODE;
    outBuf[idx++] = CRSF_ADDRESS_TX;
    outBuf[idx++] = CRSF_ADDRESS_FC;

    memcpy(&outBuf[idx], modeText, textLen);
    idx += textLen;
    outBuf[idx++] = '\0';

    uint8_t crc = crc8DvbS2Block(&outBuf[2], idx - 2);
    outBuf[idx++] = crc;

    return idx;
}

// ============================================================================
// Telemetry: Battery sensor frame (0x08)
// ============================================================================
size_t crsfBuildBattery(uint8_t* outBuf, size_t bufLen,
                        float voltage, float current,
                        uint32_t capacityMah, uint8_t remainingPct) {
    // Payload: voltage(2) + current(2) + capacity(3) + remaining(1) = 8 bytes
    // Frame: sync + length + type + payload + crc = 12
    if (bufLen < 12) return 0;

    uint16_t voltDv = static_cast<uint16_t>(voltage * 10.0f);
    uint16_t currDa = static_cast<uint16_t>(current * 10.0f);

    size_t idx = 0;
    outBuf[idx++] = CRSF_ADDRESS_FC;
    outBuf[idx++] = 10;  // length: type(1) + payload(8) + crc(1)

    outBuf[idx++] = CRSF_FRAMETYPE_BATTERY;

    // Voltage (big-endian decivolts)
    outBuf[idx++] = (voltDv >> 8) & 0xFF;
    outBuf[idx++] = voltDv & 0xFF;

    // Current (big-endian deciamps)
    outBuf[idx++] = (currDa >> 8) & 0xFF;
    outBuf[idx++] = currDa & 0xFF;

    // Capacity used (3 bytes big-endian)
    outBuf[idx++] = (capacityMah >> 16) & 0xFF;
    outBuf[idx++] = (capacityMah >> 8) & 0xFF;
    outBuf[idx++] = capacityMah & 0xFF;

    // Remaining %
    outBuf[idx++] = remainingPct;

    // CRC over type + payload
    uint8_t crc = crc8DvbS2Block(&outBuf[2], idx - 2);
    outBuf[idx++] = crc;

    return idx;
}

} // namespace melty
