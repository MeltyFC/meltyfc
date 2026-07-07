// MeltyFC — CruxF405HD Target Configuration
// Board-specific bindings — implements interfaces from include/interfaces.h

#pragma once

#include "pinmap.h"

// Clock configuration — provided by target, called from main
#ifdef __cplusplus
extern "C" {
#endif
void SystemClock_Config(void);
#ifdef __cplusplus
}
#endif

#define TARGET_NAME "CruxF405HD"
#define TARGET_MCU "STM32F405RGT6"
#define TARGET_CLOCK_MHZ 168
#define TARGET_FLASH_KB 1024
#define TARGET_RAM_KB 192

// Feature availability on this target
#define HAS_BIDIR_DSHOT 1
#define HAS_SPI_FLASH 1
#define HAS_VBAT_SENSE 1       // Verify from dump
#define HAS_CURRENT_SENSE 0    // Verify from dump
#define HAS_DEDICATED_BEEPER 0 // Uses DShot beacon instead

// I2C config
#define I2C_INSTANCE I2C2
#define I2C_SPEED_HZ 400000 // Fast mode

// DShot config — Finding 2: bitrate MUST be in Hz, not mode number
#define DSHOT_BITRATE_HZ 300000U // DShot300 = 300 kbit/s
#define DSHOT_BIDIR 1            // Bidirectional for eRPM telemetry
