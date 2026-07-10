// MeltyFC — MicoAir H743 V2 Pin Map
// DERIVED FROM BETAFLIGHT UNIFIED CONFIG (docs/pinmap_sources/MICOAIR743V2_betaflight_config.h)
// CROSS-REFERENCED: ArduPilot hwdef (docs/pinmap_sources/MICOAIR743V2_ardupilot_hwdef.dat)
// MCU: STM32H743VIT6
//
// All pin assignments below are derived from the Betaflight unified-target config,
// cross-referenced against the ArduPilot hwdef for timer/AF confirmation.
// They are one grade stronger than guessing, one grade weaker than a live dump.
// Step 0 on real hardware: `dump all` + `resource list` → diff against this file.
//
// Finding 1 gate: define PINMAP_VERIFIED_FROM_BETAFLIGHT_DUMP after live verification.
// verify.sh warns on BF_CONFIG_DERIVED; MUST be resolved before first hardware flash.

#pragma once

#define PINMAP_IS_BF_CONFIG_DERIVED 1

// ============================================================================
// CRITICAL: H743 PWR SUPPLY CONFIGURATION
// Betaflight source (system_stm32h7xx.c) confirms: H743 uses PWR_LDO_SUPPLY.
// H743 is a legacy H7 device without SMPS — wrong setting = chip never boots.
// Initial voltage scale: PWR_REGULATOR_VOLTAGE_SCALE1 (SystemClock may set VOS0).
// ============================================================================
#define H7_PWR_IS_LDO 1  // BF_CONFIG_DERIVED — H743 is LDO-only (no SMPS)

// ============================================================================
// BF_CONFIG_DERIVED PIN ASSIGNMENTS — VERIFY ON LIVE DUMP AT ARRIVAL
// ============================================================================

// Motor outputs (DShot300 via timer+DMA)
// BF config + ArduPilot hwdef cross-ref:
// Motors 1-4 on TIM1 (PE14/PE13/PE11/PE9), Motors 5-6 on TIM3 (PB1/PB0)
// ArduPilot confirms: PE14=TIM1_CH4, PE13=TIM1_CH3, PE11=TIM1_CH2, PE9=TIM1_CH1
// ArduPilot marks BIDIR on: PE14(M1), PE11(M3), PB1(M5), PD12(M7)
#define MOTOR1_PIN PE14  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR2_PIN PE13  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR3_PIN PE11  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR4_PIN PE9   // BF_CONFIG_DERIVED — verify on live dump at arrival

// Motor timer assignments — from BF + ArduPilot timer mapping
#define MOTOR1_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR2_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR3_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR4_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival

// IMU (SPI3) — BF: GYRO_1_SPI_INSTANCE SPI3
// ArduPilot: BMI270 on SPI3, CS=PA15, DRDY=PB7
// Board also has BMI088 on SPI2 (ArduPilot hwdef) — secondary IMU, not in BF unified config
#define IMU_SPI SPI3       // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_CS_PIN PA15    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_SCK_PIN PB3    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MISO_PIN PB4   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MOSI_PIN PD6   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_EXTI_PIN PB7   // BF_CONFIG_DERIVED — verify on live dump at arrival

// Secondary IMU (SPI2) — BMI088 from ArduPilot hwdef, not in BF unified config
// SPI2: SCK=PD3, MISO=PC2, MOSI=PC3, Gyro_CS=PD5, Accel_CS=PD4
// MeltyFC may use this as redundancy — kept here for reference
#define IMU2_SPI SPI2       // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU2_GYRO_CS PD5    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU2_ACCEL_CS PD4   // BF_CONFIG_DERIVED — verify on live dump at arrival

// I2C — two buses available
// I2C1: magnetometer (MAG_I2C_INSTANCE I2CDEV_1)
// I2C2: barometer (BARO_I2C_INSTANCE I2CDEV_2)
// ArduPilot: I2C_ORDER I2C2 I2C1, baro SPL06 on I2C2:0x77
#define I2C1_SCL_PIN PB8   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define I2C1_SDA_PIN PB9   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define I2C2_SCL_PIN PB10  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define I2C2_SDA_PIN PB11  // BF_CONFIG_DERIVED — verify on live dump at arrival
// H3LIS331 addresses (hardware jumper — MeltyFC-specific, not from BF)
#define H3LIS_ADDR_INNER 0x18 // Default (SDO low)
#define H3LIS_ADDR_OUTER 0x19 // SDO jumpered high

// WS2812 LED strip
// BF: PD14, TIMER_PIN_MAP( 8, LED_STRIP_PIN, 1, 12) → TIM4_CH3, DMA opt 12
// ArduPilot: PD14 TIM4_CH3 TIM4 PWM(11) GPIO(60)
#define LED_STRIP_PIN PD14        // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_TIMER TIM4      // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_DMA_OPT 12      // BF_CONFIG_DERIVED — verify on live dump at arrival

// UART6 — CRSF (ExpressLRS receiver) — ArduPilot: SERIALRX on USART6
#define CRSF_UART USART6   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_TX_PIN PC6    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_RX_PIN PC7    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_BAUD 420000

// D1: Battery voltage sense — ADC route tuple
// BF: DEFAULT_VOLTAGE_METER_SCALE 211 → divider ratio ~21.1
#define VBAT_PIN PC0                 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define VBAT_DIVIDER_RATIO 21.1f     // BF_CONFIG_DERIVED — verify on live dump at arrival
#define VBAT_ADC_INSTANCE ADC3       // H743: PC0 on ADC3 (ADC1/2 also possible but BF uses ADC3)
#define VBAT_ADC_CHANNEL ADC_CHANNEL_10  // PC0 = ADC3_INP10
#define VBAT_GPIO_PORT GPIOC
#define VBAT_GPIO_PIN GPIO_PIN_0
#define VBAT_SAMPLE_TIME ADC_SAMPLETIME_64CYCLES_5

// Current sense (ADC) — BF: DEFAULT_CURRENT_METER_SCALE 707
#define CURRENT_PIN PC1              // BF_CONFIG_DERIVED — verify on live dump at arrival

// SDMMC (blackbox via TF card slot) — 4-bit SDIO
// BF: SDIO_USE_4BIT 1, SDIO_DEVICE SDIODEV_1, DEFAULT_BLACKBOX_DEVICE SDCARD
// ArduPilot: SDMMC1 pins confirmed
#define SDMMC_CK_PIN PC12           // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_CMD_PIN PD2           // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_D0_PIN PC8            // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_D1_PIN PC9            // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_D2_PIN PC10           // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_D3_PIN PC11           // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SDMMC_USE_4BIT 1            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Beeper — inverted output per BF config
#define BEEPER_PIN PD15              // BF_CONFIG_DERIVED — verify on live dump at arrival
#define BEEPER_INVERTED 1            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Status LEDs — RGB
#define LED_RED_PIN PE3              // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_GREEN_PIN PE2            // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_BLUE_PIN PE4             // BF_CONFIG_DERIVED — verify on live dump at arrival

// USB — STM32H743 uses PA11 (USB_DM) / PA12 (USB_DP) — fixed by silicon

// ============================================================================
// I-11b: D2 SRAM guard — DMA buffers MUST reside in D2 SRAM on H743
// H743 DTCM (0x20000000) is CPU-private — DMA1/DMA2 CANNOT reach it!
// All peripheral-DMA buffers go in D2 SRAM (0x30000000-0x3001FFFF).
// MPU non-cacheable region over .d2_dma section eliminates cache maintenance.
// Call this in every driver init with each DMA buffer pointer.
// ============================================================================
#define ASSERT_IN_D2(ptr)                                                                              \
    do {                                                                                               \
        const uint32_t _addr = (uint32_t)(ptr);                                                        \
        if (_addr < 0x30000000u || _addr > 0x3001FFFFu) {                                              \
            /* DMA buffer NOT in D2 SRAM — DMA cannot reach it */                                    \
            while (1)                                                                                  \
                ;                                                                                      \
        }                                                                                              \
    } while (0)

// ============================================================================
// R3-6: NVIC Priority Table — set explicitly at init, not left to defaults.
// Lower number = higher priority. M7 has 4 bits = 16 levels.
// ============================================================================
#define NVIC_PRIO_MOTOR_DMA 1 // DShot + bidir turnaround — highest
#define NVIC_PRIO_LED_DMA 3   // WS2812 — high but below motors
#define NVIC_PRIO_I2C 5       // H3LIS331 sensor reads
#define NVIC_PRIO_SPI 5       // IMU + SPI peripherals
#define NVIC_PRIO_SDMMC 6     // SD card blackbox — below sensors, above UART
#define NVIC_PRIO_UART 7      // CRSF
#define NVIC_PRIO_USB 10      // CDC serial — low, never preempts DMA
#define NVIC_PRIO_SYSTICK 15  // Lowest — timing only

// ============================================================================
// A1: Motor Route Table (H7 — uses DMAMUX request IDs, not fixed streams)
// ============================================================================
#define MOTOR1_CHANNEL TIM_CHANNEL_4
#define MOTOR2_CHANNEL TIM_CHANNEL_3
#define MOTOR3_CHANNEL TIM_CHANNEL_2
#define MOTOR4_CHANNEL TIM_CHANNEL_1
#define MOTOR1_AF 1   // AF1 = TIM1
#define MOTOR2_AF 1
#define MOTOR3_AF 1
#define MOTOR4_AF 1
#define MOTOR1_GPIO_PORT GPIOE
#define MOTOR2_GPIO_PORT GPIOE
#define MOTOR3_GPIO_PORT GPIOE
#define MOTOR4_GPIO_PORT GPIOE
#define MOTOR1_GPIO_PIN GPIO_PIN_14
#define MOTOR2_GPIO_PIN GPIO_PIN_13
#define MOTOR3_GPIO_PIN GPIO_PIN_11
#define MOTOR4_GPIO_PIN GPIO_PIN_9
#define MOTOR1_DMAMUX_REQUEST DMA_REQUEST_TIM1_CH4
#define MOTOR2_DMAMUX_REQUEST DMA_REQUEST_TIM1_CH3
#define MOTOR3_DMAMUX_REQUEST DMA_REQUEST_TIM1_CH2
#define MOTOR4_DMAMUX_REQUEST DMA_REQUEST_TIM1_CH1

#define LED_STRIP_CHANNEL TIM_CHANNEL_3
#define LED_STRIP_AF 2
#define LED_STRIP_GPIO_PORT GPIOD
#define LED_STRIP_GPIO_PIN GPIO_PIN_14
#define LED_STRIP_DMAMUX_REQUEST DMA_REQUEST_TIM4_CH3
