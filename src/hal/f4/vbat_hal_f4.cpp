// MeltyFC — VBAT HAL: STM32F4 Family
// 12-bit ADC (0-4095), 3.3V reference, per-target divider ratio.
// [2026-07-09] R7-1: Provides senses for the LVC brain.
// [2026-07-10] D1: Consumes VBAT route tuples from pinmap.h.

#ifndef NATIVE_BUILD

#include "hal/common/vbat_hal.h"
#include "hal/common/gpio_port_clock.h"
#include "hal/common/gpio_init.h"

#ifdef STM32F4xx
#include "stm32f4xx_hal.h"
#include "target.h"

// D1: Compile-time gate — pinmap must define VBAT route tuple
#if defined(HAS_VBAT_SENSE) && HAS_VBAT_SENSE
#if !defined(VBAT_ADC_INSTANCE) || !defined(VBAT_ADC_CHANNEL) || !defined(VBAT_GPIO_PORT) || !defined(VBAT_GPIO_PIN)
#error "D1: VBAT route tuple incomplete — pinmap.h must define VBAT_ADC_INSTANCE, VBAT_ADC_CHANNEL, VBAT_GPIO_PORT, VBAT_GPIO_PIN"
#endif
#endif

namespace melty {
namespace hal {

static ADC_HandleTypeDef hAdc;
static bool adcInitialized = false;

static constexpr uint16_t ADC_FULL_SCALE = 4095;  // 12-bit
static constexpr uint16_t ADC_REF_MV = 3300;       // 3.3V reference

void vbatInit() {
    adcInitialized = false;

#if defined(HAS_VBAT_SENSE) && HAS_VBAT_SENSE
    // G-4: GPIO init through wrapper (clock enable + DSB + init)
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = VBAT_GPIO_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    meltyGpioInit(VBAT_GPIO_PORT, &gpio);

    // Enable ADC clock — determine from instance
    if (VBAT_ADC_INSTANCE == ADC1) __HAL_RCC_ADC1_CLK_ENABLE();

    hAdc.Instance = VBAT_ADC_INSTANCE;
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

    // D1: Configure VBAT channel from pinmap route tuple
    ADC_ChannelConfTypeDef chCfg = {};
    chCfg.Channel = VBAT_ADC_CHANNEL;
    chCfg.Rank = 1;
#ifdef VBAT_SAMPLE_TIME
    chCfg.SamplingTime = VBAT_SAMPLE_TIME;
#else
    chCfg.SamplingTime = ADC_SAMPLETIME_56CYCLES;
#endif
    HAL_ADC_ConfigChannel(&hAdc, &chCfg);
#endif // HAS_VBAT_SENSE
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
    uint32_t adcMv = (uint32_t)raw * ADC_REF_MV / ADC_FULL_SCALE;
#ifdef VBAT_DIVIDER_RATIO
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
