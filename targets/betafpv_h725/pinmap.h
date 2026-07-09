// MeltyFC — BetaFPV H725 Pin Map
// DERIVED FROM BETAFLIGHT UNIFIED CONFIG (docs/pinmap_sources/BETAFPVH725_betaflight_config.h)
// MCU: STM32H725RGV6
//
// B1 RULE: H7_PWR_SUPPLY is REQUIRED per-board, NO DEFAULT.
// Value comes from BF system source + board schematic ONLY.
// BetaFPV H725 boards use SMPS — verified from BF system_stm32h7xx.c
// which checks STM32H72x/H73x and applies PWR_SMPS_1V8_SUPPLIES_LDO.

#pragma once

#define PINMAP_IS_BF_CONFIG_DERIVED 1

// ============================================================================
// CRITICAL: H725 PWR SUPPLY CONFIGURATION
// H72x/H73x boards VARY between SMPS and LDO. Wrong = won't boot.
// BetaFPV H725: SMPS design (efficiency-optimized board).
// Provenance: BF system_stm32h7xx.c H72x branch + board schematic.
// ============================================================================
// H7_PWR_SUPPLY used in clock_config.c where HAL headers are included
// Value: PWR_SMPS_1V8_SUPPLIES_LDO (SMPS board design)
// Defined as a marker here; actual HAL constant used in clock_config.c
#define H7_PWR_IS_SMPS 1  // BF_CONFIG_DERIVED — board uses SMPS, not LDO-only

// Motor outputs
// BF: MOTOR1=PC6, MOTOR2=PC7, MOTOR3=PB0, MOTOR4=PB1
// Timer map: PC6=TIM8_CH1, PC7=TIM8_CH2, PB0=TIM3_CH3, PB1=TIM3_CH4
#define MOTOR1_PIN PC6   // BF_CONFIG_DERIVED
#define MOTOR2_PIN PC7   // BF_CONFIG_DERIVED
#define MOTOR3_PIN PB0   // BF_CONFIG_DERIVED
#define MOTOR4_PIN PB1   // BF_CONFIG_DERIVED
#define MOTOR1_TIMER TIM8 // BF_CONFIG_DERIVED
#define MOTOR2_TIMER TIM8 // BF_CONFIG_DERIVED
#define MOTOR3_TIMER TIM3 // BF_CONFIG_DERIVED
#define MOTOR4_TIMER TIM3 // BF_CONFIG_DERIVED

// IMU (SPI1) — ICM42688P
#define IMU_SPI SPI1       // BF_CONFIG_DERIVED
#define IMU_CS_PIN PA4     // BF_CONFIG_DERIVED
#define IMU_SCK_PIN PA5    // BF_CONFIG_DERIVED
#define IMU_MISO_PIN PA6   // BF_CONFIG_DERIVED
#define IMU_MOSI_PIN PA7   // BF_CONFIG_DERIVED

// I2C1 — H3LIS331 sensors
#define I2C_SCL_PIN PB8    // BF_CONFIG_DERIVED
#define I2C_SDA_PIN PB9    // BF_CONFIG_DERIVED
#define H3LIS_ADDR_INNER 0x18
#define H3LIS_ADDR_OUTER 0x19

// WS2812 LED strip
#define LED_STRIP_PIN PB10       // BF_CONFIG_DERIVED
#define LED_STRIP_TIMER TIM2     // BF_CONFIG_DERIVED

// UART2 — CRSF
#define CRSF_UART USART2   // BF_CONFIG_DERIVED
#define CRSF_TX_PIN PA2    // BF_CONFIG_DERIVED
#define CRSF_RX_PIN PA3    // BF_CONFIG_DERIVED
#define CRSF_BAUD 420000

// Battery ADC
#define VBAT_PIN PC0                 // BF_CONFIG_DERIVED
#define VBAT_DIVIDER_RATIO 11.0f     // BF_CONFIG_DERIVED
#define CURRENT_PIN PC1              // BF_CONFIG_DERIVED

// SPI flash (blackbox) — M25P16 on SPI2
#define FLASH_SPI SPI2               // BF_CONFIG_DERIVED
#define FLASH_CS_PIN PB6             // BF_CONFIG_DERIVED

// Beeper
#define BEEPER_PIN PC9               // BF_CONFIG_DERIVED
#define BEEPER_INVERTED 0

// Status LED
#define STATUS_LED_PIN PC5           // BF_CONFIG_DERIVED

// I-11b: D2 SRAM guard — same as H743 but different size
#define ASSERT_IN_D2(ptr)                                                      \
    do {                                                                       \
        const uint32_t _addr = (uint32_t)(ptr);                                \
        if (_addr < 0x30000000u || _addr > (0x30000000u + H7_D2_SRAM_SIZE - 1)) { \
            while (1);                                                         \
        }                                                                      \
    } while (0)

// NVIC priorities
#define NVIC_PRIO_MOTOR_DMA 1
#define NVIC_PRIO_LED_DMA 3
#define NVIC_PRIO_I2C 5
#define NVIC_PRIO_SPI 5
#define NVIC_PRIO_UART 7
#define NVIC_PRIO_USB 10
#define NVIC_PRIO_SYSTICK 15
