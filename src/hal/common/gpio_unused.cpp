// MeltyFC — Unused GPIO Init Implementation (E7 / C12 / L9)
// Sets unused GPIO pins to analog mode (lowest power, no noise coupling).
// Per-board pin lists will be customized at integration after Step 0 dump
// confirms which pins are actually unused.

#ifndef NATIVE_BUILD

#include "hal/common/gpio_unused.h"
#include "hal/common/gpio_port_clock.h"

#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif

namespace melty {
namespace hal {

void gpioInitUnused() {
    // TODO at integration: populate with the actual unused pin list
    // per board after Step 0 dump confirms which pins are routed.
    //
    // Example:
    // gpioEnablePortClock(GPIOD);
    // GPIO_InitTypeDef gpio = {};
    // gpio.Mode = GPIO_MODE_ANALOG;
    // gpio.Pull = GPIO_NOPULL;
    // gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3; // unused pins
    // HAL_GPIO_Init(GPIOD, &gpio);
    //
    // For now: no-op. This function EXISTS and is CALLED — the pin list
    // gets filled when the board's actual routing is known.
}

} // namespace hal
} // namespace melty

#endif // NATIVE_BUILD
