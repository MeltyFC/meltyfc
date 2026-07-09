// MeltyFC — DMA Buffer Placement Helpers
// Each chip family has different rules for where DMA buffers can live:
//   F4: anywhere EXCEPT CCM (0x10000000) — invariant I-11
//   F7: DTCM only (0x20000000-0x2000FFFF) — invariant I-11a
//   H7: D2 SRAM only (0x30000000-0x3001FFFF) — invariant I-11b
//
// This header provides both the section attributes and assert macros.
// Self-contained — no dependency on target pinmap.h.

#pragma once

#include <cstdint>

// ============================================================================
// DMA buffer placement — section attribute + boot assert per family
// ============================================================================

#if defined(STM32F4xx) || defined(STM32F405xx)
    // F4: default SRAM placement is fine; CCM (0x10000000) is the danger zone
    #define DMA_BUFFER_ATTR
    #define DMA_BUFFER_ASSERT(ptr) do { \
        volatile uintptr_t _addr = (uintptr_t)(ptr); \
        while ((_addr & 0xFF000000U) == 0x10000000U) {} /* CCM = hang -> IWDG */ \
    } while(0)

#elif defined(STM32F7xx) || defined(STM32F722xx) || defined(STM32F745xx)
    // F7: force into DTCM (0x20000000-0x2000FFFF) — never cached, DMA-accessible
    #define DMA_BUFFER_ATTR __attribute__((section(".dtcm_dma")))
    #define DMA_BUFFER_ASSERT(ptr) do { \
        volatile uintptr_t _addr = (uintptr_t)(ptr); \
        while (_addr < 0x20000000U || _addr > 0x2000FFFFU) {} /* not DTCM = hang -> IWDG */ \
    } while(0)

#elif defined(STM32H7xx) || defined(STM32H743xx)
    // H7: force into D2 SRAM — DMA1/DMA2 reachable, DTCM is CPU-private
    // Single source of truth for D2 range — used by assert, MPU config, and linker
    #define H7_D2_SRAM_BASE  0x30000000U
    #define H7_D2_SRAM_SIZE  0x00020000U  // 128KB (D2 SRAM1 + SRAM2 on H743)
    #define H7_D2_SRAM_END   (H7_D2_SRAM_BASE + H7_D2_SRAM_SIZE - 1U)
    #define DMA_BUFFER_ATTR __attribute__((section(".d2_dma")))
    #define DMA_BUFFER_ASSERT(ptr) do { \
        volatile uintptr_t _addr = (uintptr_t)(ptr); \
        while (_addr < H7_D2_SRAM_BASE || _addr > H7_D2_SRAM_END) {} /* not D2 = hang -> IWDG */ \
    } while(0)

#elif defined(NATIVE_BUILD) || defined(UNIT_TEST)
    // Native test builds — no DMA, no placement constraints
    #define DMA_BUFFER_ATTR
    #define DMA_BUFFER_ASSERT(ptr) ((void)(ptr))

#else
    #error "Unknown chip family — define DMA buffer placement in dma_buf.h"
#endif
