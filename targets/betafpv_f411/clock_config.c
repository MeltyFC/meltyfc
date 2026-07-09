// MeltyFC — BetaFPV F411 Clock Configuration
// HSE = 8MHz, SYSCLK = 96MHz (not 100 — USB requires exact 48MHz)
// CS-1: At 100MHz no integer PLLQ yields 48MHz. F411 has no HSI48/CRS.
// PLLM=4, PLLN=192 -> VCO 384, PLLP=4 -> 96MHz, PLLQ=8 -> USB 48.000 exact.
// RM0383: VOS Scale1 supports up to 100MHz. 96MHz is within spec.
// Flash: 3 wait states (>90MHz at 3.3V, RM0383 Table 6)

#ifndef NATIVE_BUILD
#ifdef STM32F4xx

#include "stm32f4xx_hal.h"

void SystemClock_Config(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL: 8MHz / 4 * 192 = 384MHz VCO -> /4 = 96MHz SYSCLK
    // RM0383: VCO must be 100-432MHz. 384 is within range.
    osc.PLL.PLLM = 4;      // VCO input = 8/4 = 2MHz (RM0383: 1-2MHz)
    osc.PLL.PLLN = 192;    // VCO = 2*192 = 384MHz (RM0383: 100-432MHz) ✓
    osc.PLL.PLLP = RCC_PLLP_DIV4;  // SYSCLK = 384/4 = 96MHz ✓
    osc.PLL.PLLQ = 8;      // USB = 384/8 = 48.000MHz exact ✓
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;    // 96MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV2;     // 48MHz (RM0383: max 50MHz) ✓
    clk.APB2CLKDivider = RCC_HCLK_DIV1;     // 96MHz (RM0383: max 100MHz) ✓
    // Flash: 3 wait states (>90MHz at 3.3V, RM0383 Table 6) ✓
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);

    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

#endif // STM32F4xx
#endif // NATIVE_BUILD
