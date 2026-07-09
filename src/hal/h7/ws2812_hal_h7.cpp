// MeltyFC — WS2812 HAL: STM32H7 Family (H743)
// Timer+DMA driver for WS2812B LED strip.
// Same D2 SRAM + MPU non-cacheable approach as DShot.
//
// STUB — implementation BLOCKED on Step 0 hardware dump.

#ifndef NATIVE_BUILD

#include "hal/common/ws2812_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32H7xx
#include "stm32h7xx_hal.h"

namespace melty {
namespace hal {

// WS2812 compare buffer — in D2 SRAM (invariant I-11b)
// 24 bits per LED * max LEDs + reset pulse
static DMA_BUFFER_ATTR uint16_t ws2812Buf[24 * 32 + 50]; // 32 LEDs max + reset

void ws2812Init() {
    DMA_BUFFER_ASSERT(ws2812Buf);
    // TODO P2: Configure LED timer + DMAMUX from pinmap.h
    // TODO P2: MPU region already configured in dshotInit — verify overlap
}

void ws2812Send(const uint8_t* colorData, uint16_t numLeds) {
    (void)colorData;
    (void)numLeds;
    // TODO P2: Encode GRB data to compare buffer, trigger DMA
}

bool ws2812Busy() {
    return false;
}

} // namespace hal
} // namespace melty

#endif // STM32H7xx
#endif // NATIVE_BUILD
