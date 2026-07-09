// MeltyFC — H7 System Init: MPU Configuration
// meltyH7MpuInit() configures the MPU non-cacheable region over D2 SRAM
// for DMA coherency. Called from dshotInit() before any DMA buffer use.
//
// SystemClock_Config lives in targets/<board>/clock_config.c (per-board)
// — NOT here. Only meltyH7MpuInit is family-level (same MPU setup for all H7).

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx_hal.h"
#include "hal/common/dma_buf.h"

// MPU region setup for D2 SRAM — called before any DMA init
// Makes the .d2_dma section non-cacheable so DMA transfers are coherent
// without per-transfer clean/invalidate calls in hot paths
void meltyH7MpuInit(void) {
    HAL_MPU_Disable();

    MPU_Region_InitTypeDef mpu_init = {};
    mpu_init.Enable = MPU_REGION_ENABLE;
    mpu_init.Number = MPU_REGION_NUMBER0;
    mpu_init.BaseAddress = H7_D2_SRAM_BASE;   // from dma_buf.h — single source of truth
    mpu_init.Size = MPU_REGION_SIZE_128KB;     // matches H7_D2_SRAM_SIZE
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

#endif // STM32H7xx
#endif // NATIVE_BUILD
