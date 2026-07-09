// MeltyFC — WS2812 HAL: STM32F7 Family (F722, F745)
// Timer+DMA driver for WS2812 LED strip. Double-buffered.
//
// Same timer+DMA approach as F4, but DMA buffers in DTCM (invariant I-11a).
//
// STUB — implementation BLOCKED on Step 0 hardware dump.
// LED_STRIP_PIN, LED_STRIP_TIMER from target pinmap.h.
// [2026-07-09] Created for T1 F722 HAL port.

#ifndef NATIVE_BUILD

#include "hal/common/ws2812_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32F7xx
#include "stm32f7xx_hal.h"

namespace melty {
namespace hal {

static uint16_t pixelCount_ = 0;

// WS2812 timing: 800kHz data rate
// Bit period = timer_clock / 800000
// Bit 1 high = 68% of period (~850ns)
// Bit 0 high = 32% of period (~400ns)
// Reset pulse = >50us low (handled by trailing zeros in DMA buffer)

void ws2812Init(uint16_t maxPixels) {
    pixelCount_ = maxPixels;
    // TODO P1: Allocate DMA buffer in DTCM (24 bits/pixel * maxPixels + reset padding)
    // TODO P1: Configure timer + DMA from pinmap.h
}

void ws2812SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    (void)index;
    (void)r;
    (void)g;
    (void)b;
    // TODO P1: Write GRB to compare buffer (WS2812 byte order is G-R-B)
}

void ws2812Show() {
    // TODO P1: Trigger DMA transfer
    // No cache maintenance needed — DTCM is never cached.
}

bool ws2812Busy() {
    return false;
}

uint16_t ws2812PixelCount() {
    return pixelCount_;
}

} // namespace hal
} // namespace melty

#endif // STM32F7xx
#endif // NATIVE_BUILD
