// MeltyFC — DWT Cycle Counter with 64-bit Wrap Extension (A3 + C6/I-32)
// CYCCNT wraps at ~10-25 seconds. The 2kHz loop guarantees call frequency
// far exceeds the wrap period, so software extension is reliable.

#pragma once

#include <cstdint>

#ifndef NATIVE_BUILD
#if defined(STM32F4xx)
#include "stm32f4xx.h"
#elif defined(STM32F7xx)
#include "stm32f7xx.h"
#elif defined(STM32H7xx)
#include "stm32h7xx.h"
#endif
#endif

namespace melty {

#ifndef NATIVE_BUILD

// State for the 64-bit extension
struct DwtState {
    uint32_t lastRaw;
    uint64_t totalCycles;
};

// Module-level state — call micros() at least once per CYCCNT wrap period
extern DwtState dwtState;

inline void dwtInit() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    dwtState.lastRaw = 0;
    dwtState.totalCycles = 0;
}

// C6/I-32: Read microseconds with 64-bit wrap-safe extension
// Must be called at least once per CYCCNT wrap period (~10-25s)
// The 2kHz control loop guarantees this.
inline uint32_t micros() {
    uint32_t now = DWT->CYCCNT;
    uint32_t delta = now - dwtState.lastRaw; // Unsigned wrap-safe
    dwtState.lastRaw = now;
    dwtState.totalCycles += delta;
    return (uint32_t)(dwtState.totalCycles / (SystemCoreClock / 1000000U));
}

// Read raw 32-bit cycle count (for sub-µs deltas within one wrap period)
inline uint32_t cycleCount() {
    return DWT->CYCCNT;
}

// Convert cycle delta to microseconds
inline uint32_t cyclesToMicros(uint32_t cycles) {
    return cycles / (SystemCoreClock / 1000000U);
}

// Read 64-bit total cycle count (for long-duration timing)
inline uint64_t totalCycles() {
    // Update the extension first
    uint32_t now = DWT->CYCCNT;
    uint32_t delta = now - dwtState.lastRaw;
    dwtState.lastRaw = now;
    dwtState.totalCycles += delta;
    return dwtState.totalCycles;
}

#else // NATIVE_BUILD

inline void dwtInit() {}
inline uint32_t micros() { return 0; }
inline uint32_t cycleCount() { return 0; }
inline uint32_t cyclesToMicros(uint32_t cycles) { return cycles; }
inline uint64_t totalCycles() { return 0; }

#endif

} // namespace melty
