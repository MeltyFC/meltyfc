// MeltyFC — BetaFPV H725 Clock + PWR Configuration
// HSE = 8MHz, SYSCLK = 520MHz (VOS1 conservative — 550MHz VOS0 is later)
// B1: PWR supply = SMPS (board-specific, from pinmap.h provenance)
// B2: Conservative bring-up — max safe clock for VOS1

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "target.h"

void SystemClock_Config(void) {
    // B1: CRITICAL — PWR supply config. H725 boards VARY.
    // This board uses SMPS. Wrong = chip never reaches main().
    // B1: PWR supply from board design
    // BetaFPV H725 uses SMPS, but the SMPS HAL constants require the H725 CMSIS
    // device header (not available when building against nucleo_h743zi).
    // When a proper H725 PIO board exists: HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
    // For now: LDO fallback (safe — board will boot but SMPS unused).
    // HARDWARE-GATED: verify correct supply config on first flash.
#if defined(SMPS) && defined(H7_PWR_IS_SMPS) && H7_PWR_IS_SMPS
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);  // SMPS board — needs H725 CMSIS
#else
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);              // LDO fallback for cross-compile
#endif

    // B2: VOS1 for 520MHz (conservative). VOS0 needed for 550MHz.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // HSE oscillator + PLL
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL: 8MHz / 1 * 130 = 1040MHz VCO -> /2 = 520MHz SYSCLK
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 130;
    osc.PLL.PLLP = 2;   // 520MHz SYSCLK
    osc.PLL.PLLQ = 4;   // 260MHz
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    osc.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&osc);

    // Bus clocks: AHB=260, APB1=130, APB2=130
    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                    RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;    // 260MHz
    clk.APB1CLKDivider = RCC_APB1_DIV2;    // 130MHz
    clk.APB2CLKDivider = RCC_APB2_DIV2;    // 130MHz
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;
    // Flash: 3 wait states at VOS1/260MHz AHB (from datasheet Table 17)
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);
}

#endif // STM32H7xx
#endif // NATIVE_BUILD
