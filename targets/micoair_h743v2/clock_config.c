// MeltyFC — MicoAir H743 V2 Clock + PWR Configuration
// HSE = 8MHz, SYSCLK = 400MHz (VOS1), AHB = 200MHz, APB1/APB2 = 100MHz
// PWR: LDO supply (H743 has no SMPS — wrong setting = won't boot)
// Derived from BF system_stm32h7xx.c + ArduPilot hwdef

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "target.h"

void SystemClock_Config(void) {
    // CRITICAL: PWR supply configuration — must be first
    // H743 = LDO only. SMPS config = chip never reaches main()
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);  // H7_PWR_IS_LDO from pinmap.h

    // Voltage scaling: VOS1 for 400MHz operation
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // HSE oscillator enable (8MHz crystal from BF config)
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL1: 8MHz / 1 * 100 = 800MHz VCO → /2 = 400MHz SYSCLK
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 100;
    osc.PLL.PLLP = 2;   // SYSCLK = 400MHz
    osc.PLL.PLLQ = 4;   // 200MHz for peripherals
    osc.PLL.PLLR = 2;   // unused
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;  // 8MHz input
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;  // Wide VCO for 800MHz
    osc.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&osc);

    // Bus clocks: AHB=200, APB1=100, APB2=100
    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                    RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;    // 200MHz
    clk.APB1CLKDivider = RCC_APB1_DIV2;    // 100MHz
    clk.APB2CLKDivider = RCC_APB2_DIV2;    // 100MHz
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;
    // Flash latency: 2 wait states at VOS1 + 200MHz AHB
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

#endif // STM32H7xx
#endif // NATIVE_BUILD
