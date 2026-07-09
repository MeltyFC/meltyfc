// MeltyFC — Aikon F7 Mini Clock Configuration
// HSE = 8MHz, SYSCLK = 216MHz, AHB = 216MHz, APB1 = 54MHz, APB2 = 108MHz
// Timer clocks: APB1 timers = 108MHz, APB2 timers = 216MHz
// Derived from BF unified config (HSE_VALUE=8000000)
// [2026-07-09] R6-2: Real clock tree, not placeholder

#ifndef NATIVE_BUILD
#ifdef STM32F7xx

#include "stm32f7xx_hal.h"

void SystemClock_Config(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // HSE oscillator + PLL
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL: 8MHz / 8 * 432 = 432MHz VCO -> /2 = 216MHz SYSCLK
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 432;
    osc.PLL.PLLP = RCC_PLLP_DIV2;  // 216MHz
    osc.PLL.PLLQ = 9;               // 48MHz for USB
    HAL_RCC_OscConfig(&osc);

    // Activate OverDrive for 216MHz
    HAL_PWREx_EnableOverDrive();

    // Bus clocks
    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;    // 216MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV4;     // 54MHz (timers = 108MHz)
    clk.APB2CLKDivider = RCC_HCLK_DIV2;     // 108MHz (timers = 216MHz)
    // Flash: 7 wait states at 216MHz
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_7);

    // ART accelerator + prefetch (F7 performance feature)
    __HAL_FLASH_ART_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

#endif // STM32F7xx
#endif // NATIVE_BUILD
