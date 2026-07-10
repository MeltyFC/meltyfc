// MeltyFC — GPIO Port Clock Enable Helper (A2 / I-25)
// Call before ANY HAL_GPIO_Init.
//
// ES0287 §2.2.7 (F411, applies family-wide to F4 class):
// "Delay after an RCC peripheral clock enabling"
// First register access after clock-enable can be lost.
// Workaround: DSB or dummy-read of the enable register.
// The dummy-read is included unconditionally — costs nothing, prevents the erratum.

#pragma once

#ifndef NATIVE_BUILD

#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif

static inline void gpioEnablePortClock(GPIO_TypeDef* port) {
    if (port == GPIOA) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    else if (port == GPIOD) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
    else if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
#ifdef GPIOF
    else if (port == GPIOF) { __HAL_RCC_GPIOF_CLK_ENABLE(); }
#endif
#ifdef GPIOG
    else if (port == GPIOG) { __HAL_RCC_GPIOG_CLK_ENABLE(); }
#endif
#ifdef GPIOH
    else if (port == GPIOH) { __HAL_RCC_GPIOH_CLK_ENABLE(); }
#endif

    // ES0287 §2.2.7: Dummy-read after clock enable to prevent first-access loss.
    // The __HAL_RCC_GPIOx_CLK_ENABLE macros already include a dummy-read in newer
    // HAL versions, but we add an explicit DSB for safety across all HAL versions.
    __DSB();
}

#endif // NATIVE_BUILD
