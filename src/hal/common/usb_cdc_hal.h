// MeltyFC — USB CDC HAL Contract (E3)
// Virtual COM port for CLI + config. CS-2 provides the USB kernel clock.
//
// I-9: USB writes MUST be guarded — never write to a disconnected port.
// The guard templates in main.cpp become real code here.
//
// Implementation uses the stm32cube USB Device middleware (CDC class).
// This header defines what MeltyFC needs; the per-family implementation
// wraps the middleware calls.

#pragma once

#include <cstdint>

namespace melty {
namespace hal {

// Initialize USB CDC device stack.
// Requires: USB kernel clock configured (CS-2: HSI48+CRS on H7, PLLQ on F4/F7).
// Returns true if USB device enumerated successfully.
bool usbCdcInit();

// Check if a USB host is connected and the port is open.
// I-9: ALL write calls must check this first.
bool usbCdcConnected();

// Write data to the USB CDC port.
// Returns number of bytes actually written (may be less if buffer full).
// Returns 0 if not connected (I-9 guard).
uint16_t usbCdcWrite(const uint8_t* data, uint16_t len);

// Write a null-terminated string.
uint16_t usbCdcPrint(const char* str);

// Read available data from the USB CDC RX buffer.
// Returns number of bytes read.
uint16_t usbCdcRead(uint8_t* buf, uint16_t maxLen);

// Returns number of bytes available to read.
uint16_t usbCdcAvailable();

// Flush the TX buffer (blocking, with timeout).
bool usbCdcFlush(uint32_t timeoutMs);

} // namespace hal
} // namespace melty
