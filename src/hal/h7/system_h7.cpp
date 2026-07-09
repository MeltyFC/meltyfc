// MeltyFC — H7 System Init: PWR, Clock, MPU, Memory Domains
// This file handles the H743-specific boot traps:
//   1. PWR supply config (LDO vs SMPS — wrong = won't boot)
//   2. Clock at 400MHz VOS1 (safe; 480MHz VOS0 is later experiment)
//   3. MPU non-cacheable region over D2 SRAM for DMA coherency
//   4. D-cache enable (with MPU region protecting DMA buffers)
//
// STUB — implementation BLOCKED on Step 0 hardware dump.

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"

// MPU region setup for D2 SRAM — called before any DMA init
// Makes the .d2_dma section non-cacheable so DMA transfers are coherent
// without per-transfer clean/invalidate calls in hot paths
void meltyH7MpuInit(void) {
    HAL_MPU_Disable();

    MPU_Region_InitTypeDef mpu_init = {};
    mpu_init.Enable = MPU_REGION_ENABLE;
    mpu_init.Number = MPU_REGION_NUMBER0;
    mpu_init.BaseAddress = 0x30000000;          // D2 SRAM start
    mpu_init.Size = MPU_REGION_SIZE_32KB;        // Covers .d2_dma section
    mpu_init.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_init.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    mpu_init.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    mpu_init.IsShareable = MPU_ACCESS_SHAREABLE;
    mpu_init.TypeExtField = MPU_TEX_LEVEL1;
    mpu_init.SubRegionDisable = 0;
    mpu_init.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&mpu_init);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

// H743 clock config — 400MHz, VOS1
// Uses HSE from the board's oscillator (value from pinmap.h / build flags)
// 480MHz (VOS0) is a one-line experiment for later — not a bring-up risk
void SystemClock_Config(void) {
    // TODO P2: Full clock tree from BF/ArduPilot reference
    // Key settings:
    //   - HSE = 8MHz (from BF config)
    //   - PLL1: HSE/1 * 100 = 400MHz → SYSCLK
    //   - AHB = SYSCLK/2 = 200MHz
    //   - APB1 = AHB/2 = 100MHz
    //   - APB2 = AHB/2 = 100MHz
    //   - Flash latency: 2 wait states at VOS1/200MHz AHB

    // PWR config — CRITICAL: H743 is LDO only (no SMPS)
    // Wrong setting = chip never reaches main()
    // HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);  // from pinmap.h: H743_PWR_SUPPLY

    // VOS1 for 400MHz operation
    // __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
}

#endif // STM32H7xx
#endif // NATIVE_BUILD
