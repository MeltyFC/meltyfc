// MeltyFC — VBAT HAL: STM32H7 Family
// 16-bit ADC (0-65535) — NOT 12-bit like F4/F7!
// R7-1: Code that assumes 4095 on H7 reads 16x high → LVC never trips.
// [2026-07-09]
// [2026-07-10] D1: Consumes VBAT route tuples from pinmap.h.

#ifndef NATIVE_BUILD
#include "hal/common/vbat_hal.h"
#include "hal/common/gpio_port_clock.h"
#include "hal/common/gpio_init.h"
#ifdef STM32H7xx
#include "stm32h7xx_hal.h"
#include "target.h"

// D1: Compile-time gate
#if defined(HAS_VBAT_SENSE) && HAS_VBAT_SENSE
#if !defined(VBAT_ADC_INSTANCE) || !defined(VBAT_ADC_CHANNEL) || !defined(VBAT_GPIO_PORT) || !defined(VBAT_GPIO_PIN)
#error "D1: VBAT route tuple incomplete — pinmap.h must define VBAT_ADC_INSTANCE, VBAT_ADC_CHANNEL, VBAT_GPIO_PORT, VBAT_GPIO_PIN"
#endif
#endif

namespace melty {
namespace hal {

static ADC_HandleTypeDef hAdc;
static bool adcInitialized = false;

// H7: 16-bit ADC! This is the family-specific trap.
static constexpr uint16_t ADC_FULL_SCALE = 65535;  // 16-bit
static constexpr uint16_t ADC_REF_MV = 3300;

void vbatInit() {
    adcInitialized = false;

#if defined(HAS_VBAT_SENSE) && HAS_VBAT_SENSE
    // G-4: GPIO init through wrapper (clock enable + DSB + init)
    GPIO_InitTypeDef gpio = {};
    gpio.Pin = VBAT_GPIO_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    meltyGpioInit(VBAT_GPIO_PORT, &gpio);

    // H7 ADC clock — enable based on instance from route tuple
    if (VBAT_ADC_INSTANCE == ADC3) __HAL_RCC_ADC3_CLK_ENABLE();
    else __HAL_RCC_ADC12_CLK_ENABLE();

    hAdc.Instance = VBAT_ADC_INSTANCE;
    hAdc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV6;
    hAdc.Init.Resolution = ADC_RESOLUTION_16B;  // 16-bit — the H7 difference
    hAdc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hAdc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hAdc.Init.LowPowerAutoWait = DISABLE;
    hAdc.Init.ContinuousConvMode = DISABLE;
    hAdc.Init.NbrOfConversion = 1;
    hAdc.Init.DiscontinuousConvMode = DISABLE;
    hAdc.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hAdc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hAdc) == HAL_OK) {
        // H7 ADC requires calibration
        HAL_ADCEx_Calibration_Start(&hAdc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
        adcInitialized = true;
    }

    // D1: Channel from pinmap route tuple
    ADC_ChannelConfTypeDef chCfg = {};
    chCfg.Channel = VBAT_ADC_CHANNEL;
    chCfg.Rank = ADC_REGULAR_RANK_1;
    chCfg.SingleDiff = ADC_SINGLE_ENDED;
    chCfg.OffsetNumber = ADC_OFFSET_NONE;
#ifdef VBAT_SAMPLE_TIME
    chCfg.SamplingTime = VBAT_SAMPLE_TIME;
#else
    chCfg.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
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
    // 16-bit conversion: mV = raw * 3300 / 65535 * divider
    uint32_t adcMv = (uint32_t)raw * ADC_REF_MV / ADC_FULL_SCALE;
#ifdef VBAT_DIVIDER_RATIO
    return (uint32_t)((float)adcMv * VBAT_DIVIDER_RATIO);
#else
    return adcMv;
#endif
}

bool vbatValid() { return adcInitialized; }
uint16_t vbatAdcFullScale() { return ADC_FULL_SCALE; }

} // namespace hal
} // namespace melty
#endif
#endif
