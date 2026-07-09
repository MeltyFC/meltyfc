// MeltyFC — DMA Buffer Placement Helpers
// Each chip family has different rules for where DMA buffers can live:
//   F4: anywhere EXCEPT CCM (0x10000000) — invariant I-11
//   F7: DTCM only (0x20000000-0x2000FFFF) — invariant I-11a
//   H7: D2 SRAM only — size varies by chip (I-11b):
//       H743: 128KB (0x30000000-0x3001FFFF)
//       H723/H725: 32KB (0x30000000-0x30007FFF)
//       H7_D2_SRAM_SIZE MUST be defined per target (-D flag or target.h)
//
// This header provides section attributes, assert macros, and #error
// gates for required per-chip constants. Self-contained.

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

#elif defined(STM32H7xx) || defined(STM32H743xx) || defined(STM32H723xx) || defined(STM32H725xx)
    // H7: force into D2 SRAM — DMA1/DMA2 reachable, DTCM is CPU-private
    // D2 size varies by chip — must be defined per target in target.h/pinmap.h
    // H743: 128KB (SRAM1 64K + SRAM2 64K + SRAM3 32K... varies by package)
    // H723/H725: 32KB D2 SRAM
    #ifndef H7_D2_SRAM_BASE
        #define H7_D2_SRAM_BASE  0x30000000U  // Same across all H7
    #endif
    #ifndef H7_D2_SRAM_SIZE
        #error "H7_D2_SRAM_SIZE must be defined per target (H743=0x20000, H723/H725=0x8000)"
    #endif
    // B1: PWR supply — every H7 target must declare its power source.
    // H7_PWR_IS_SMPS=1 for SMPS boards, 0 or undefined for LDO-only.
    // clock_config.c uses this to call the correct HAL_PWREx_ConfigSupply().
    // No default — a future H7 target forgetting this gets a build error.
    #if !defined(H7_PWR_IS_SMPS) && !defined(H7_PWR_IS_LDO)
        #error "H7 target must define H7_PWR_IS_SMPS=1 (SMPS board) or H7_PWR_IS_LDO=1 (LDO-only)"
    #endif
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
