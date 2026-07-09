// MeltyFC — BetaFPV H725 Target Configuration
// MCU: STM32H725RGV6
// 550MHz capable (VOS0) — bring up at 520MHz VOS1 (conservative)

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

#define TARGET_NAME "BetaFPV_H725"
#define TARGET_MCU "STM32H725RGV6"
#define TARGET_CLOCK_MHZ 520     // VOS1 conservative; 550MHz (VOS0) is later experiment
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
#define MELTYFC_TIER MELTYFC_TIER_FULL

// B1: H7_PWR_SUPPLY is defined in pinmap.h (REQUIRED, no default)
// B2: Clock at 520MHz VOS1 (conservative). 550MHz VOS0 = later.
// B3: D2 SRAM = 32KB (not 128KB like H743!)
