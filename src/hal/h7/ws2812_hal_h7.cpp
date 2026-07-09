// MeltyFC — WS2812 HAL: STM32H7 Family (H743)
// D2 SRAM buffers + MPU non-cacheable + DMAMUX routing.
// [2026-07-09] G2: Driver body written. Matches hal/common/ws2812_hal.h interface.

#ifndef NATIVE_BUILD

#include "hal/common/ws2812_hal.h"
#include "hal/common/dma_buf.h"

#ifdef STM32H7xx
#include "stm32h7xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

// Timer clock: 200MHz (APB1 timers on H743 at 400MHz SYSCLK)
// Actually APB1 timer clock depends on prescaler. At AHB=200, APB1=100, timers=200MHz
// WS2812: 800kHz → period = 200/0.8 = 250 ticks
static constexpr uint16_t WS_PERIOD = 250;
static constexpr uint16_t WS_BIT1   = 160;  // 0.8us high
static constexpr uint16_t WS_BIT0   = 80;   // 0.4us high
static constexpr uint16_t WS_RESET  = 50;
static constexpr uint16_t MAX_PX    = 32;

// DMA buffer in D2 SRAM (I-11b). MPU non-cacheable region handles coherency.
static DMA_BUFFER_ATTR uint16_t compareBuf[MAX_PX * 24 + WS_RESET];
static uint8_t pixelRgb[MAX_PX * 3];
static uint16_t numPx = 0;
static volatile bool busy = false;
static TIM_HandleTypeDef hTim;
static DMA_HandleTypeDef hDma;

// DMAMUX request ID for LED timer (from H743 ref manual)
// TIM4_CH3 (MicoAir LED pin PD14) → DMAMUX request depends on channel
static constexpr uint32_t DMAMUX_LED_REQUEST = 52; // TIM4_CH3 — verify from ref manual

void ws2812Init(uint16_t maxPixels) {
    DMA_BUFFER_ASSERT(compareBuf);
    numPx = (maxPixels > MAX_PX) ? MAX_PX : maxPixels;

    uint32_t len = numPx * 24 + WS_RESET;
    for (uint32_t i = 0; i < len; i++) compareBuf[i] = 0;
    for (uint16_t i = 0; i < numPx * 3; i++) pixelRgb[i] = 0;

    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

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
    HAL_TIM_PWM_ConfigChannel(&hTim, &oc, TIM_CHANNEL_3); // CH3 for PD14

    // DMAMUX routing
    hDma.Instance = DMA1_Stream5;  // Pick an unused stream
    hDma.Init.Request = DMAMUX_LED_REQUEST;
    hDma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hDma.Init.PeriphInc = DMA_PINC_DISABLE;
    hDma.Init.MemInc = DMA_MINC_ENABLE;
    hDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hDma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hDma.Init.Mode = DMA_NORMAL;
    hDma.Init.Priority = DMA_PRIORITY_MEDIUM;
    hDma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hDma);
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
    // No cache maintenance — MPU non-cacheable region
    busy = true;
    HAL_TIM_PWM_Start_DMA(&hTim, TIM_CHANNEL_3, (uint32_t*)compareBuf, idx);
}

bool ws2812Busy() { return busy; }
uint16_t ws2812PixelCount() { return numPx; }

} // namespace hal
} // namespace melty

#endif // STM32H7xx
#endif // NATIVE_BUILD
