// MeltyFC — WS2812 HAL Contract
// Each chip family implements timer+DMA WS2812 output.
// Double-buffered from day one (spec mandate).

#pragma once

#include <cstdint>

namespace melty {
namespace hal {

// Initialize WS2812 timer+DMA on the LED_STRIP_PIN defined in target pinmap.
// maxPixels sets the buffer size — must match physical LED count.
void ws2812Init(uint16_t maxPixels);

// Set a single pixel in the DMA buffer (GRB byte order, WS2812 native).
// Does NOT trigger a transfer — call ws2812Show() after setting all pixels.
void ws2812SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

// Trigger DMA transfer of the pixel buffer to the LED strip.
// Non-blocking — returns immediately. Check ws2812Busy() before next show().
void ws2812Show();

// Returns true if the previous DMA transfer is still in progress.
bool ws2812Busy();

// Returns the configured pixel count.
uint16_t ws2812PixelCount();

} // namespace hal
} // namespace melty
