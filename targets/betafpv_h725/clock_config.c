// MeltyFC — BetaFPV H725 Clock + PWR Configuration
// HSE = 8MHz, SYSCLK = 520MHz at VOS0 (standard max for H723/H725)
// B1: PWR supply = SMPS (board-specific, from pinmap.h provenance)
//
// RM0468 voltage scaling:
//   VOS0: up to 520MHz (standard), 550MHz (boost) — H723/H725 specific
//   VOS1: up to 400MHz — same as H743
//   VOS2: up to 300MHz
//   VOS3: up to 170MHz
//
// Cross-ref audit: previous config claimed VOS1 at 520MHz — that's 30% over
// the VOS1 400MHz limit. Fixed to VOS0 which is the correct scale for 520MHz.
// Flash latency was already correct for VOS0 (consistent by accident).

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "target.h"

void SystemClock_Config(void) {
    // B1: PWR supply from board design
#if defined(SMPS) && defined(H7_PWR_IS_SMPS) && H7_PWR_IS_SMPS
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
#else
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);  // LDO fallback for cross-compile
#endif

    // VOS0 for 520MHz — REQUIRED (VOS1 caps at 400MHz per RM0468)
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    // I-6: Bounded wait for voltage scaling ready
    {
        uint32_t timeout = 10000;
        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
            if (--timeout == 0) {
                // VOS not ready — clock config will fail, I-12 catches it
                return;
            }
        }
    }

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
    // Flash: 3 wait states at VOS0/260MHz AHB (RM0468 Table 16)
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3);
}

#endif // STM32H7xx
#endif // NATIVE_BUILD
