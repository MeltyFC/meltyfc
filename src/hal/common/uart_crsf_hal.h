// MeltyFC — UART HAL for CRSF Receiver (E2)
// DMA ring-buffer RX on the CRSF UART, feeding the pure-logic parser.
// Per-target: CRSF_UART, CRSF_TX_PIN, CRSF_RX_PIN, CRSF_BAUD from pinmap.h.
//
// Architecture: UART RX DMA fills a circular ring buffer. Main loop
// drains the ring into the CRSF parser byte-by-byte. No ISR processing
// of protocol data — ISR just manages the DMA (per ISR_CONTRACT.md).

#pragma once

#include <cstdint>

namespace melty {
namespace hal {

// Initialize UART for CRSF reception.
// Configures UART at 420000 baud, 8N1, DMA RX circular buffer.
// Returns true if UART + DMA init succeeded.
bool crsfUartInit();

// Returns number of bytes available in the RX ring buffer.
// Call from main loop to check for new data.
uint16_t crsfUartAvailable();

// Read one byte from the RX ring buffer.
// Returns -1 if no data available.
int16_t crsfUartReadByte();

// Send telemetry frame (TX direction — CRSF is half-duplex on one wire).
// Blocks briefly for TX DMA completion.
bool crsfUartSend(const uint8_t* data, uint16_t len);

} // namespace hal
} // namespace melty
