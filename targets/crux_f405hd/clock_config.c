// MeltyFC — CruxF405HD Clock Configuration
// HSE = 8MHz, SYSCLK = 168MHz, AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
// Derived from RM0090 clock tree + BF unified config (HSE_VALUE=8000000)
// [2026-07-09] Cross-ref audit: was placeholder HSI — fixed to real PLL config

#ifndef NATIVE_BUILD
#ifdef STM32F4xx

#include "stm32f4xx_hal.h"

void SystemClock_Config(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // HSE oscillator + PLL
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL: 8MHz / 8 * 336 = 336MHz VCO -> /2 = 168MHz SYSCLK
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV2;  // 168MHz
    osc.PLL.PLLQ = 7;               // 48MHz for USB
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) return;

    // Bus clocks
    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;    // 168MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV4;     // 42MHz (timers = 84MHz)
    clk.APB2CLKDivider = RCC_HCLK_DIV2;     // 84MHz (timers = 168MHz)
    // Flash: 5 wait states at 168MHz/3.3V (RM0090 Table 10)
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) return;

    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

#endif // STM32F4xx
#endif // NATIVE_BUILD
