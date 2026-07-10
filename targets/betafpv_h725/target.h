// MeltyFC — BetaFPV H725 Target Configuration
// MCU: STM32H725RGV6
// MCU supports 550MHz (VOS0). Running at 400MHz VOS1 (H743 proven chain).

#pragma once

#include "pinmap.h"
#define MELTYFC_TIER MELTYFC_TIER_FULL
#include "feature_tiers.h"

#ifdef __cplusplus
extern "C" {
#endif
void SystemClock_Config(void);
#ifdef __cplusplus
}
#endif

#define TARGET_NAME "BetaFPV_H725"
#define TARGET_MCU "STM32H725RGV6"
#define TARGET_CLOCK_MHZ 400     // CS-3: copied H743 proven chain. 520 = future bench experiment.
#define TARGET_FLASH_KB 1024
#define TARGET_RAM_KB 564        // AXI 320KB + D2 32KB + D3 16KB + DTCM 128KB + ITCM 64KB

// H7 D2 SRAM size — H725 has LESS than H743! (32KB vs 128KB)
#define H7_D2_SRAM_SIZE 0x00008000U  // 32KB D2 SRAM on H723/H725

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

// Feature tier: FULL (1MB flash)

// B1: H7_PWR_SUPPLY is defined in pinmap.h (REQUIRED, no default)
// B2: Clock at 400MHz VOS1. 520MHz requires VOS0 + SMPS research — parked.
// B3: D2 SRAM = 32KB (not 128KB like H743!)
#define EXPECTED_TIMER_CLOCK_HZ 200000000U  // CS-3: 400MHz/2 = 200MHz timer clock
