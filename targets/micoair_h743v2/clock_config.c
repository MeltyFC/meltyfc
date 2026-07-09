// MeltyFC — MicoAir H743 V2 Clock + PWR Configuration
// HSE = 8MHz, SYSCLK = 400MHz (VOS1), AHB = 200MHz, APB1/APB2 = 100MHz
// PWR: LDO supply (H743 has no SMPS)
//
// RM0433 compliance (all values within limits):
//   VCO input: 8MHz (RGE_3: 8-16MHz) ✓
//   VCO: 800MHz (wide: 192-836MHz) ✓
//   SYSCLK: 400MHz (VOS1 max: 400MHz) ✓ at-cap, legal
//   AHB: 200MHz (max 240MHz) ✓
//   APB1/APB2: 100MHz (max 120MHz) ✓
//   Flash: 2WS (VOS1 at 185-210MHz AXI, RM0433 Table 17) ✓
//   USB: 48MHz via HSI48+CRS ✓

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "target.h"

void SystemClock_Config(void) {
    // CRITICAL: H743 = LDO only. SMPS = chip never reaches main()
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // VOS1 for 400MHz
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // CS-4 + I-6: Bounded VOSRDY wait
    {
        uint32_t timeout = 100000;
        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
            if (--timeout == 0) return; // I-12 catches clock mismatch
        }
    }

    // HSE + PLL + HSI48 (for USB)
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    osc.HSEState = RCC_HSE_ON;
    osc.HSI48State = RCC_HSI48_ON;  // CS-2: USB kernel clock source
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL1: 8MHz / 1 * 100 = 800MHz VCO (RM0433: wide 192-836) -> /2 = 400MHz
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 100;
    osc.PLL.PLLP = 2;   // SYSCLK = 400MHz ✓
    osc.PLL.PLLQ = 4;   // 200MHz
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;  // 8MHz input ✓
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;  // 800MHz VCO ✓
    osc.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&osc);

    // Bus clocks
    RCC_ClkInitTypeDef clk = {};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                    RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;    // 200MHz ✓
    clk.APB1CLKDivider = RCC_APB1_DIV2;    // 100MHz ✓
    clk.APB2CLKDivider = RCC_APB2_DIV2;    // 100MHz ✓
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;
    // Flash: 2WS at VOS1/200MHz AHB (RM0433 Table 17) ✓
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);

    // CS-2: USB kernel clock — HSI48 + CRS (sync-on-SOF for USB tolerance)
    RCC_PeriphCLKInitTypeDef periphClk = {};
    periphClk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periphClk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&periphClk);

    // Enable CRS for HSI48 auto-trim
    __HAL_RCC_CRS_CLK_ENABLE();
    RCC_CRSInitTypeDef crsInit = {};
    crsInit.Prescaler = RCC_CRS_SYNC_DIV1;
    crsInit.Source = RCC_CRS_SYNC_SOURCE_USB2;
    crsInit.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    crsInit.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
    crsInit.ErrorLimitValue = 34;
    crsInit.HSI48CalibrationValue = 32;
    HAL_RCCEx_CRSConfig(&crsInit);
}

#endif // STM32H7xx
#endif // NATIVE_BUILD
