// MeltyFC — Aikon F7 Mini Clock Configuration
// STM32F722 @ 216MHz with 8MHz HSE
// Derived from BF AIKONF7 config (HSE 8MHz confirmed).
// [2026-07-09] Created for T1 F722 HAL port.
//
// TODO: Validate against STM32CubeMX output when hardware arrives.
// For now, placeholder HSI config sufficient for compilation verification.

#include "stm32f7xx_hal.h"

void SystemClock_Config(void) {
    // Target: HSE 8MHz -> PLL -> SYSCLK 216MHz
    //   AHB  = 216MHz (prescaler 1)
    //   APB1 = 54MHz  (prescaler 4, max 54MHz on F722)
    //   APB2 = 108MHz (prescaler 2, max 108MHz on F722)
    //
    // Placeholder: HSI at default speed until real clock tree is validated.
    // The real config will be derived from STM32CubeMX with HSE_VALUE=8000000.

    // Enable power controller clock
    __HAL_RCC_PWR_CLK_ENABLE();

    // Configure voltage regulator scale for 216MHz
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Configure flash latency for 16MHz HSI (0 wait states)
    // Real config: 7 wait states for 216MHz on F722
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

    // Enable ART accelerator + prefetch (F7-specific performance feature)
    __HAL_FLASH_ART_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}
