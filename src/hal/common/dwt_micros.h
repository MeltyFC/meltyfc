// MeltyFC — DWT Cycle Counter Microsecond Timer (A3)
// Replaces the HAL_GetTick()*1000 stub with actual microsecond precision.
// Uses the ARM Cortex-M DWT CYCCNT register (available on M4 and M7).
//
// CYCCNT wraps at ~10-25 seconds depending on clock speed.
// Always use unsigned subtraction for deltas — never compare absolutes.
// micros() wraps at ~71 minutes (32-bit µs counter).

#pragma once

#include <cstdint>

namespace melty {

// Call once at init, AFTER SystemClock_Config
// Enables TRCENA + CYCCNTENA in the DWT
inline void dwtInit() {
#ifndef NATIVE_BUILD
    // Enable trace (required for DWT access)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Reset and enable the cycle counter
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
}

// Read microseconds since boot (wraps at ~71 minutes)
// Uses DWT->CYCCNT / (SystemCoreClock / 1000000)
inline uint32_t micros() {
#ifndef NATIVE_BUILD
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
#else
    return 0;
#endif
}

// Read raw cycle count (for sub-microsecond deltas)
// WRAPS at ~10-25 seconds. Use ONLY for short deltas.
inline uint32_t cycleCount() {
#ifndef NATIVE_BUILD
    return DWT->CYCCNT;
#else
    return 0;
#endif
}

// Convert cycle delta to microseconds
inline uint32_t cyclesToMicros(uint32_t cycles) {
#ifndef NATIVE_BUILD
    return cycles / (SystemCoreClock / 1000000U);
#else
    return cycles;
#endif
}

} // namespace melty
