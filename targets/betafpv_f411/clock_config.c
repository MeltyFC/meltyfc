// MeltyFC — BetaFPV F411 Clock Configuration
// HSE = 8MHz (from BF config), SYSCLK = 100MHz (F411 max)
// AHB = 100MHz, APB1 = 50MHz (max), APB2 = 100MHz
// Timer clocks: APB1 timers = 100MHz, APB2 timers = 100MHz

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
    // PLL: 8MHz / 4 * 100 = 200MHz VCO -> /2 = 100MHz SYSCLK
    osc.PLL.PLLM = 4;
    osc.PLL.PLLN = 100;
    osc.PLL.PLLP = RCC_PLLP_DIV2;  // 100MHz
    osc.PLL.PLLQ = 4;               // 50MHz (USB needs 48, close enough with trim)
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;    // 100MHz
    clk.APB1CLKDivider = RCC_HCLK_DIV2;     // 50MHz (timers = 100MHz)
    clk.APB2CLKDivider = RCC_HCLK_DIV1;     // 100MHz
    // Flash: 3 wait states at 100MHz
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);

    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

#endif // STM32F4xx
#endif // NATIVE_BUILD
