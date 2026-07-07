// MeltyFC — CruxF405HD Clock Configuration
// STM32F405 @ 168MHz with 8MHz HSE
// TODO: Derive from Step 0 BF dump + STM32CubeMX

#include "stm32f4xx_hal.h"

void SystemClock_Config(void) {
    // Placeholder — configures HSI at default speed until real clock tree
    // is derived from the BF dump. The real config will set:
    //   HSE 8MHz → PLL → SYSCLK 168MHz
    //   AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
    //
    // For now, HAL_Init() already sets up HSI at 16MHz which is enough
    // for the target build to link and verify module compilation.

    // Enable power controller clock
    __HAL_RCC_PWR_CLK_ENABLE();

    // Configure flash latency for 16MHz HSI (0 wait states)
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);
}
