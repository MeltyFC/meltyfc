// MeltyFC — WS2812 HAL: STM32F7 Family (F722, F745)
// Same structure as F4, DMA buffers in DTCM (I-11a).
// [2026-07-09] G2: Driver body written.

#ifndef NATIVE_BUILD

#include "hal/common/ws2812_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32F7xx
#include "stm32f7xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

// Timer clock: 108MHz (APB1 timers on F722/F745)
// WS2812: 800kHz → period = 108/0.8 = 135 ticks
static constexpr uint16_t WS_PERIOD = 135;
static constexpr uint16_t WS_BIT1   = 86;   // 0.8us high
static constexpr uint16_t WS_BIT0   = 43;   // 0.4us high
static constexpr uint16_t WS_RESET  = 50;
static constexpr uint16_t MAX_PX    = 32;

static DMA_BUFFER_ATTR uint16_t compareBuf[MAX_PX * 24 + WS_RESET];
static uint8_t pixelRgb[MAX_PX * 3];
static uint16_t numPx = 0;
static volatile bool busy = false;
static TIM_HandleTypeDef hTim;
static DMA_HandleTypeDef hDmaLed;

void ws2812Init(uint16_t maxPixels) {
    DMA_BUFFER_ASSERT(compareBuf);
    numPx = (maxPixels > MAX_PX) ? MAX_PX : maxPixels;

    uint32_t len = numPx * 24 + WS_RESET;
    for (uint32_t i = 0; i < len; i++) compareBuf[i] = 0;
    for (uint16_t i = 0; i < numPx * 3; i++) pixelRgb[i] = 0;

    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();

    hTim.Instance = LED_STRIP_TIMER;
    hTim.Init.Prescaler = 0;
    hTim.Init.CounterMode = TIM_COUNTERMODE_UP;
    hTim.Init.Period = WS_PERIOD - 1;
    hTim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hTim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&hTim);

    TIM_OC_InitTypeDef oc = {};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&hTim, &oc, TIM_CHANNEL_1);

    // I-3: explicit DMA_NORMAL — dead CPU = DMA stops = LEDs freeze
    hDmaLed.Init.Mode = DMA_NORMAL;  // I-3: never circular
    hDmaLed.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hDmaLed.Init.Priority = DMA_PRIORITY_MEDIUM;
}

void ws2812SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= numPx) return;
    pixelRgb[index * 3 + 0] = r;
    pixelRgb[index * 3 + 1] = g;
    pixelRgb[index * 3 + 2] = b;
}

void ws2812Show() {
    if (busy) return;
    uint32_t idx = 0;
    for (uint16_t p = 0; p < numPx; p++) {
        uint8_t grb[3] = { pixelRgb[p*3+1], pixelRgb[p*3+0], pixelRgb[p*3+2] };
        for (int c = 0; c < 3; c++)
            for (int b = 7; b >= 0; b--)
                compareBuf[idx++] = (grb[c] & (1 << b)) ? WS_BIT1 : WS_BIT0;
    }
    for (uint16_t i = 0; i < WS_RESET; i++) compareBuf[idx++] = 0;
    // No cache maintenance — DTCM is never cached
    busy = true;
    HAL_TIM_PWM_Start_DMA(&hTim, TIM_CHANNEL_1, (uint32_t*)compareBuf, idx);
}

bool ws2812Busy() { return busy; }
uint16_t ws2812PixelCount() { return numPx; }

} // namespace hal
} // namespace melty

#endif // STM32F7xx
#endif // NATIVE_BUILD
