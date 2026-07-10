// MeltyFC — Per-Timer Kernel Clock (A4 / I-24)
// SystemCoreClock is NOT a universal timer clock.
// F7: APB1 timers = 108MHz, APB2 timers = 216MHz (both from 216MHz SYSCLK)
// F4: APB1 timers = 84MHz, APB2 timers = 168MHz
// H7: APB1 timers = 200MHz, APB2 timers = 200MHz (at 400MHz SYSCLK)
//
// timerKernelClockHz(TIMx) returns the correct clock for any timer instance.
// The DShot driver must use this per-entry, not a single hardcoded value.

#pragma once

#ifndef NATIVE_BUILD

#include <cstdint>

#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif

static inline uint32_t timerKernelClockHz(TIM_TypeDef* tim) {
    // APB2 timers: TIM1, TIM8, TIM9, TIM10, TIM11
    // APB1 timers: TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14
    // When APBx prescaler != 1, timer clock = APBx * 2

    bool isAPB2 = (tim == TIM1)
#ifdef TIM8
        || (tim == TIM8)
#endif
#ifdef TIM9
        || (tim == TIM9)
#endif
#ifdef TIM10
        || (tim == TIM10)
#endif
#ifdef TIM11
        || (tim == TIM11)
#endif
    ;

#if defined(STM32F4xx)
    // F4: SYSCLK=168, AHB=168, APB1=42 (/4, timer×2=84), APB2=84 (/2, timer×2=168)
    // F411: SYSCLK=96, AHB=96, APB1=48 (/2, timer×2=96), APB2=96 (/1, timer=96)
    if (isAPB2) {
        uint32_t apb2 = HAL_RCC_GetPCLK2Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb2 == hclk) ? apb2 : apb2 * 2;
    } else {
        uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb1 == hclk) ? apb1 : apb1 * 2;
    }

#elif defined(STM32F7xx)
    // F7: SYSCLK=216, AHB=216, APB1=54 (/4, timer×2=108), APB2=108 (/2, timer×2=216)
    if (isAPB2) {
        uint32_t apb2 = HAL_RCC_GetPCLK2Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb2 == hclk) ? apb2 : apb2 * 2;
    } else {
        uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb1 == hclk) ? apb1 : apb1 * 2;
    }

#elif defined(STM32H7xx)
    // H7: SYSCLK=400, AHB=200, APB1=100 (/2, timer×2=200), APB2=100 (/2, timer×2=200)
    // Both domains produce the same timer clock on H7 (200MHz)
    if (isAPB2) {
        uint32_t apb2 = HAL_RCC_GetPCLK2Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb2 == hclk) ? apb2 : apb2 * 2;
    } else {
        uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
        uint32_t hclk = HAL_RCC_GetHCLKFreq();
        return (apb1 == hclk) ? apb1 : apb1 * 2;
    }
#endif
}

#endif // NATIVE_BUILD
