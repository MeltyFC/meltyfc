// MeltyFC — GPIO Init Wrapper (G-4)
// Combines gpioEnablePortClock + HAL_GPIO_Init into one call.
// verify.sh gates bare HAL_GPIO_Init in HAL files — use this instead.
// The DSB (ES0287 §2.2.7) is inside gpioEnablePortClock already.

#pragma once

#ifndef NATIVE_BUILD

#include "hal/common/gpio_port_clock.h"

static inline void meltyGpioInit(GPIO_TypeDef* port, GPIO_InitTypeDef* init) {
    gpioEnablePortClock(port);
    HAL_GPIO_Init(port, init);
}

#endif // NATIVE_BUILD
