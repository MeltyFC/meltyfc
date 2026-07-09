// MeltyFC — VBAT HAL: STM32F4 Family
// 12-bit ADC (0-4095), 3.3V reference, per-target divider ratio.
// [2026-07-09] R7-1: Provides senses for the LVC brain.

#ifndef NATIVE_BUILD

#include "hal/common/vbat_hal.h"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"
#include "target.h"

namespace melty {
namespace hal {

static ADC_HandleTypeDef hAdc;
static bool adcInitialized = false;

static constexpr uint16_t ADC_FULL_SCALE = 4095;  // 12-bit
static constexpr uint16_t ADC_REF_MV = 3300;       // 3.3V reference

void vbatInit() {
    adcInitialized = false;

    __HAL_RCC_ADC1_CLK_ENABLE();

    hAdc.Instance = ADC1;
    hAdc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hAdc.Init.Resolution = ADC_RESOLUTION_12B;
    hAdc.Init.ScanConvMode = DISABLE;
    hAdc.Init.ContinuousConvMode = DISABLE;
    hAdc.Init.DiscontinuousConvMode = DISABLE;
    hAdc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hAdc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hAdc.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hAdc) == HAL_OK) {
        adcInitialized = true;
    }

    // Configure VBAT channel
    ADC_ChannelConfTypeDef chCfg = {};
    chCfg.Channel = ADC_CHANNEL_10;  // PC0 = ADC_CHANNEL_10 — verify per pinmap
    chCfg.Rank = 1;
    chCfg.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hAdc, &chCfg);
}

uint16_t vbatReadRaw() {
    if (!adcInitialized) return 0;
    HAL_ADC_Start(&hAdc);
    if (HAL_ADC_PollForConversion(&hAdc, 2) != HAL_OK) return 0;
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hAdc);
    HAL_ADC_Stop(&hAdc);
    return raw;
}

uint32_t vbatReadMv() {
    uint16_t raw = vbatReadRaw();
    if (raw == 0) return 0;
    // Convert: mV = raw * 3300 / 4095 * divider_ratio
    uint32_t adcMv = (uint32_t)raw * ADC_REF_MV / ADC_FULL_SCALE;
#ifdef VBAT_DIVIDER_RATIO
    // VBAT_DIVIDER_RATIO is a float — multiply to get pack voltage
    return (uint32_t)((float)adcMv * VBAT_DIVIDER_RATIO);
#else
    return adcMv;
#endif
}

bool vbatValid() {
    return adcInitialized;
}

uint16_t vbatAdcFullScale() {
    return ADC_FULL_SCALE;
}

} // namespace hal
} // namespace melty

#endif // STM32F4xx
#endif // NATIVE_BUILD
