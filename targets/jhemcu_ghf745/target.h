// MeltyFC — JHEMCU GHF745 AIO Target Configuration
// Board: JHEMCU GHF745 AIO
// MCU: STM32F745VGT6
// BF target: JHEF745 (STM32F745)

#pragma once

#include "pinmap.h"
#include "feature_tiers.h"

// Clock configuration — provided by target, called from main
#ifdef __cplusplus
extern "C" {
#endif
void SystemClock_Config(void);
#ifdef __cplusplus
}
#endif

#define TARGET_NAME "JHEMCU_GHF745"
#define TARGET_MCU "STM32F745VGT6"
#define TARGET_CLOCK_MHZ 216
#define TARGET_FLASH_KB 1024
#define TARGET_RAM_KB 320

// Feature availability on this target
#define HAS_BIDIR_DSHOT 1
#define HAS_SPI_FLASH 1       // M25P16 / PY25Q128HA via SPI1
#define HAS_VBAT_SENSE 1      // PC3 ADC
#define HAS_CURRENT_SENSE 1   // PC2 ADC
#define HAS_DEDICATED_BEEPER 1 // PD15, inverted
#define HAS_BARO 1             // BMP280/DPS310 via I2C1
#define HAS_OSD 0              // MAX7456 exists on BF but MeltyFC doesn't use it

// I2C config — F7 uses TIMINGR-based I2C IP (not F4's CCR/TRISE)
// Arduino Wire abstracts this; see PORTING.md
#define I2C_INSTANCE I2C1
#define I2C_SPEED_HZ 400000 // Fast mode

// DShot config — Finding 2: bitrate MUST be in Hz, not mode number
#define DSHOT_BITRATE_HZ 300000U // DShot300 = 300 kbit/s
#define DSHOT_BIDIR 1            // Bidirectional for eRPM telemetry

// F7 cache note: L1 I/D caches present. DMA buffers placed in DTCM
// (never cached, DMA-accessible via AHBS) — no cache-maintenance needed.
// See ASSERT_IN_DTCM() in pinmap.h (invariant I-11a).
#define MELTYFC_TIER MELTYFC_TIER_FULL
