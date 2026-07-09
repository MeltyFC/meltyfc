// MeltyFC — MicoAir H743 V2 Target Configuration
// Board: MicoAir H743 V2 AIO 45A
// MCU: STM32H743VIT6
// BF target: MICOAIR743V2 (STM32H743)
// ArduPilot target: MicoAir743v2

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

#define TARGET_NAME "MicoAirH743V2"
#define TARGET_MCU "STM32H743VIT6"
#define TARGET_CLOCK_MHZ 400   // VOS1 @ 400MHz — safe bring-up; 480MHz (VOS0) is later experiment
#define TARGET_FLASH_KB 2048
#define TARGET_RAM_KB 1024     // ~1MB across domains (AXI SRAM 512KB + D2 288KB + D3 64KB + DTCM 128KB + ITCM 64KB)

// Feature availability on this target
#define HAS_BIDIR_DSHOT 1
#define HAS_SPI_FLASH 0         // No SPI flash — blackbox is on SDMMC TF card
#define HAS_SDMMC_BLACKBOX 1    // TF card slot via SDMMC1, 4-bit
#define HAS_VBAT_SENSE 1        // PC0 ADC, scale 211 (divider ~21.1:1)
#define HAS_CURRENT_SENSE 1     // PC1 ADC, scale 707
#define HAS_DEDICATED_BEEPER 1  // PD15, inverted
#define HAS_BARO 1              // DPS310 via I2C2 (BF) / SPL06 via I2C2 (ArduPilot) — verify on arrival
#define HAS_DUAL_IMU 1          // BMI270 (SPI3) + BMI088 (SPI2) — ArduPilot confirms both
#define HAS_RGB_LED 1           // PE3=R, PE2=G, PE4=B
#define HAS_OSD 0               // MAX7456 exists on BF but MeltyFC doesn't use it

// I2C config — H743 uses TIMINGR-based I2C IP (same as F7, not F4's CCR/TRISE)
// Arduino Wire abstracts this; see PORTING.md
// I2C1 = magnetometer bus, I2C2 = barometer bus
#define I2C_INSTANCE I2C2       // Primary for MeltyFC H3LIS331 sensors
#define I2C_SPEED_HZ 400000     // Fast mode

// DShot config — Finding 2: bitrate MUST be in Hz, not mode number
#define DSHOT_BITRATE_HZ 300000U // DShot300 = 300 kbit/s
#define DSHOT_BIDIR 1            // Bidirectional for eRPM telemetry

// H743 memory architecture notes:
// - DTCM (0x20000000): CPU-private, DMA CANNOT reach — stack goes here (fast)
// - AXI SRAM (0x24000000): .data/.bss go here
// - D2 SRAM (0x30000000): all DMA buffers — MPU non-cacheable region applied
// - D3 SRAM (0x38000000): BDMA-accessible, unused in v1
// See ASSERT_IN_D2() in pinmap.h (invariant I-11b).
// See H743_PWR_SUPPLY in pinmap.h — CRITICAL for boot.

// H743 DMAMUX note: request routing is programmable (unlike F4/F7 fixed
// stream+channel mapping). DShot/WS2812 drivers gain a DMAMUX-assignment
// layer in the h7 family dir. This is the NICE difference.
