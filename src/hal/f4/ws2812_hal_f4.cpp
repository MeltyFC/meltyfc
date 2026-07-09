// MeltyFC — WS2812 HAL: STM32F4 Family (F405)
// Timer+DMA driver for WS2812 LED strip. Double-buffered.
//
// STUB — implementation BLOCKED on Step 0 hardware dump.
// LED_STRIP_PIN and LED_STRIP_TIMER come from target pinmap.h.

#ifndef NATIVE_BUILD

#include "hal/common/ws2812_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"

namespace melty {
namespace hal {

static uint16_t pixelCount_ = 0;

void ws2812Init(uint16_t maxPixels) {
    pixelCount_ = maxPixels;
    // TODO P1: Configure timer + DMA from pinmap.h
    // TODO P1: Allocate double-buffer (must NOT be in CCM)
}

void ws2812SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    (void)index;
    (void)r;
    (void)g;
    (void)b;
    // TODO P1: Write GRB to compare buffer
}

void ws2812Show() {
    // TODO P1: Trigger DMA transfer
}

bool ws2812Busy() {
    return false;
}

uint16_t ws2812PixelCount() {
    return pixelCount_;
}

} // namespace hal
} // namespace melty

#endif // STM32F4xx
#endif // NATIVE_BUILD
