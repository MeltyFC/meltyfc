// MeltyFC — Motor Route Table (A1)
// Table-driven timer/DMA/GPIO routing for DShot and WS2812.
// Each target defines its route entries in pinmap.h; the HAL drivers
// iterate the table instead of hardcoding timer instances.
//
// This eliminates the class of bugs where the HAL assumes TIM1×4
// but the board uses TIM3+TIM4 split or TIM8+TIM3 split.

#pragma once

#include <cstdint>

// Forward declarations — resolved by the HAL headers per family
#ifndef NATIVE_BUILD
#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif
#endif

namespace melty {

// Maximum motors supported by the route table
static constexpr uint8_t MAX_MOTORS = 4;

#ifndef NATIVE_BUILD

// One entry per motor: which timer, which channel, which DMA, which GPIO AF
struct MotorRouteEntry {
    TIM_TypeDef* timer;         // e.g., TIM1, TIM3, TIM8
    uint32_t channel;           // TIM_CHANNEL_1..4
    uint8_t gpioAF;             // GPIO alternate function number
    GPIO_TypeDef* gpioPort;     // e.g., GPIOC
    uint16_t gpioPin;           // e.g., GPIO_PIN_6
#if defined(STM32H7xx)
    uint32_t dmamuxRequest;     // DMA_REQUEST_TIMx_CHy (H7 DMAMUX)
#else
    DMA_Stream_TypeDef* dmaStream; // DMA stream instance (F4/F7 fixed mapping)
    uint32_t dmaChannel;        // DMA channel select (F4/F7)
#endif
};

// LED route: same structure but only one entry
struct LedRouteEntry {
    TIM_TypeDef* timer;
    uint32_t channel;
    uint8_t gpioAF;
    GPIO_TypeDef* gpioPort;
    uint16_t gpioPin;
#if defined(STM32H7xx)
    uint32_t dmamuxRequest;
#else
    DMA_Stream_TypeDef* dmaStream;
    uint32_t dmaChannel;
#endif
};

// Helpers
inline bool timerNeedsMOE(TIM_TypeDef* tim) {
    // TIM1 and TIM8 have break/MOE; others don't
    return (tim == TIM1)
#ifdef TIM8
        || (tim == TIM8)
#endif
    ;
}

// Count unique timers in a route table (for multi-timer init)
uint8_t countUniqueTimers(const MotorRouteEntry* routes, uint8_t count);

// Get the list of unique timer pointers (max 4, typically 1-2)
uint8_t getUniqueTimers(const MotorRouteEntry* routes, uint8_t count,
                        TIM_TypeDef** out, uint8_t outMax);

#endif // NATIVE_BUILD

} // namespace melty

// E3/I-27: Required route defines — compile error if ANY motor tuple member missing
#ifndef NATIVE_BUILD
#ifndef MOTOR1_TIMER
    #error "MOTOR1_TIMER not defined — add full route tuple to target pinmap.h"
#endif
#ifndef MOTOR1_CHANNEL
    #error "MOTOR1_CHANNEL not defined — add full route tuple to target pinmap.h"
#endif
#ifndef MOTOR1_AF
    #error "MOTOR1_AF not defined — add full route tuple to target pinmap.h"
#endif
#ifndef MOTOR1_GPIO_PORT
    #error "MOTOR1_GPIO_PORT not defined — add full route tuple to target pinmap.h"
#endif
#ifndef MOTOR1_GPIO_PIN
    #error "MOTOR1_GPIO_PIN not defined — add full route tuple to target pinmap.h"
#endif
// Repeat for LED strip
#ifndef LED_STRIP_TIMER
    #error "LED_STRIP_TIMER not defined — add LED route to target pinmap.h"
#endif
#ifndef LED_STRIP_CHANNEL
    #error "LED_STRIP_CHANNEL not defined — add LED route to target pinmap.h"
#endif
#endif // NATIVE_BUILD
