// MeltyFC — BetaFPV H725 Clock + PWR Configuration
// HSE = 8MHz, SYSCLK = 400MHz at VOS1 (copied from H743's proven chain)
//
// CS-3: Previous config had THREE stacked violations:
//   (a) VCO 1040MHz > 836MHz wide-range cap
//   (b) 520MHz on VOS1 which caps at 400MHz
//   (c) No USB kernel clock
// Fix: copy H743's verified 400MHz/VOS1 chain. Park 520MHz as a future
// bench experiment requiring VOS0 + SMPS/VOS0 constraint research.
//
// RM0468 compliance (all values within limits):
//   VCO input: 8MHz (RGE_3: 8-16MHz) ✓
//   VCO: 800MHz (wide: 192-836MHz) ✓
//   SYSCLK: 400MHz (VOS1 max: 400MHz) ✓ at-cap, legal
//   AHB: 200MHz (max 275MHz) ✓
//   APB1/APB2: 100MHz (max 137.5MHz) ✓
//   Flash: 2WS (VOS1 at 185-210MHz AHB, RM0468 Table 16) ✓

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "target.h"

void SystemClock_Config(void) {
    // B1: PWR supply from board design
#if defined(SMPS) && defined(H7_PWR_IS_SMPS) && H7_PWR_IS_SMPS
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
#else
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
#endif

    // VOS1 for 400MHz (genuinely conservative — at-cap, not over)
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // I-6 + CS-4: Bounded VOSRDY wait
    {
        uint32_t timeout = 100000;
        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
            if (--timeout == 0) return; // I-12 catches the clock mismatch
        }
    }

    // HSE + PLL — identical to H743's proven chain
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    osc.HSEState = RCC_HSE_ON;
    osc.HSI48State = RCC_HSI48_ON;  // CS-2: USB kernel clock source
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    // PLL: 8MHz / 1 * 100 = 800MHz VCO (RM0468: wide 192-836) -> /2 = 400MHz
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 100;
    osc.PLL.PLLP = 2;   // 400MHz SYSCLK
    osc.PLL.PLLQ = 4;   // 200MHz
    osc.PLL.PLLR = 2;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;  // 8MHz input (8-16MHz range) ✓
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;  // Wide VCO for 800MHz ✓
    osc.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&osc);

    // Bus clocks — same as H743
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
    // Flash: 2WS at VOS1/200MHz AHB (RM0468 Table 16) ✓
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);

    // CS-2: USB kernel clock — HSI48 + CRS (sync-on-SOF for USB tolerance)
    RCC_PeriphCLKInitTypeDef periphClk = {};
    periphClk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    periphClk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&periphClk);

    // Enable CRS for HSI48 auto-trim (sync to USB SOF)
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
