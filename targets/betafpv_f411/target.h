// MeltyFC — BetaFPV F411 Target Configuration
// Board: BetaFPV F411 AIO (whoop class)
// MCU: STM32F411CEU6

#pragma once

#include "pinmap.h"
#include "feature_tiers.h"

#ifdef __cplusplus
extern "C" {
#endif
void SystemClock_Config(void);
#ifdef __cplusplus
}
#endif

#define TARGET_NAME "BetaFPV_F411"
#define TARGET_MCU "STM32F411CEU6"
#define TARGET_CLOCK_MHZ 96      // CS-1: 96 not 100 — USB needs exact 48MHz from PLLQ
#define TARGET_FLASH_KB 512
#define TARGET_RAM_KB 128

#define HAS_BIDIR_DSHOT 1
#define HAS_SPI_FLASH 1
#define HAS_VBAT_SENSE 1
#define HAS_CURRENT_SENSE 1
#define HAS_DEDICATED_BEEPER 1

// I2C config
#define I2C_INSTANCE I2C1
#define I2C_SPEED_HZ 400000

// DShot config
#define DSHOT_BITRATE_HZ 300000U
#define DSHOT_BIDIR 1

// Feature tier: STD (512KB flash — POV fonts capped, blackbox reduced)
#define MELTYFC_TIER MELTYFC_TIER_STD

// A3 NOTE: F411 timer/DMA constraints
// F411 has 5 timers with DMA: TIM1, TIM2, TIM3, TIM4, TIM5
// But only DMA2 can access memory-to-peripheral for TIM1
// Motors use TIM3+TIM4 (2 channels each) — both on DMA1
// LED uses TIM1_CH1 — needs DMA2_Stream1 or Stream3
// This means LED DMA and motor DMA use DIFFERENT DMA controllers
// which avoids the stream-sharing problem that plagues F405.
// Verify allocation on hardware.
#define EXPECTED_TIMER_CLOCK_HZ 96000000U   // CS-1: 96MHz (APB2 prescaler=1, no x2)
