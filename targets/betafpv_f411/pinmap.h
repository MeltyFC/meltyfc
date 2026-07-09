// MeltyFC — BetaFPV F411 Pin Map
// DERIVED FROM BETAFLIGHT UNIFIED CONFIG (docs/pinmap_sources/BETAFPVF411_betaflight_config.h)
// MCU: STM32F411CEU6
//
// All pin assignments derived from the Betaflight unified-target config.
// One grade stronger than guessing, one grade weaker than a live dump.
// Step 0: `dump all` + `resource list` -> diff against this file.

#pragma once

#define PINMAP_IS_BF_CONFIG_DERIVED 1

// Motor outputs (DShot300 via timer+DMA)
// BF config: MOTOR1=PB4(TIM3_CH1), MOTOR2=PB5(TIM3_CH2),
//            MOTOR3=PB6(TIM4_CH1), MOTOR4=PB7(TIM4_CH2)
// A3 WATCH: F411 has fewer DMA streams than F405!
// TIM3+TIM4 = two timers for 4 motors (2 channels each)
// DMA allocation must be verified — F411 has DMA2 only for some peripherals
#define MOTOR1_PIN PB4   // BF_CONFIG_DERIVED
#define MOTOR2_PIN PB5   // BF_CONFIG_DERIVED
#define MOTOR3_PIN PB6   // BF_CONFIG_DERIVED
#define MOTOR4_PIN PB7   // BF_CONFIG_DERIVED
#define MOTOR1_TIMER TIM3 // BF_CONFIG_DERIVED
#define MOTOR2_TIMER TIM3 // BF_CONFIG_DERIVED
#define MOTOR3_TIMER TIM4 // BF_CONFIG_DERIVED
#define MOTOR4_TIMER TIM4 // BF_CONFIG_DERIVED

// IMU (SPI1) — MPU6000 or BMI270 depending on revision
#define IMU_SPI SPI1       // BF_CONFIG_DERIVED
#define IMU_CS_PIN PA4     // BF_CONFIG_DERIVED
#define IMU_SCK_PIN PA5    // BF_CONFIG_DERIVED
#define IMU_MISO_PIN PA6   // BF_CONFIG_DERIVED
#define IMU_MOSI_PIN PA7   // BF_CONFIG_DERIVED
#define IMU_EXTI_PIN PA1   // BF_CONFIG_DERIVED

// I2C1 — H3LIS331 sensors
#define I2C_SCL_PIN PB8    // BF_CONFIG_DERIVED
#define I2C_SDA_PIN PB9    // BF_CONFIG_DERIVED
#define H3LIS_ADDR_INNER 0x18
#define H3LIS_ADDR_OUTER 0x19

// WS2812 LED strip
// BF: PA8 — TIM1_CH1
// A3: This shares TIM1 with potential DShot use — verify no conflict
#define LED_STRIP_PIN PA8        // BF_CONFIG_DERIVED
#define LED_STRIP_TIMER TIM1     // BF_CONFIG_DERIVED

// UART2 — CRSF (ExpressLRS)
#define CRSF_UART USART2   // BF_CONFIG_DERIVED
#define CRSF_TX_PIN PA2    // BF_CONFIG_DERIVED
#define CRSF_RX_PIN PA3    // BF_CONFIG_DERIVED
#define CRSF_BAUD 420000

// Battery voltage sense (ADC)
#define VBAT_PIN PB0                 // BF_CONFIG_DERIVED
#define VBAT_DIVIDER_RATIO 11.0f     // BF_CONFIG_DERIVED

// Current sense (ADC)
#define CURRENT_PIN PB1              // BF_CONFIG_DERIVED

// SPI flash (blackbox) — M25P16 on SPI2
#define FLASH_SPI SPI2               // BF_CONFIG_DERIVED
#define FLASH_CS_PIN PB12            // BF_CONFIG_DERIVED

// Beeper
#define BEEPER_PIN PB2               // BF_CONFIG_DERIVED
#define BEEPER_INVERTED 0            // BF_CONFIG_DERIVED

// Status LEDs
#define STATUS_LED_PIN PC13          // BF_CONFIG_DERIVED
#define STATUS_LED2_PIN PC14         // BF_CONFIG_DERIVED

// F4 family: DMA buffers must NOT be in CCM
// F411 has no CCM — all SRAM is DMA-accessible. Assert still present for safety.
#define ASSERT_NOT_CCM(ptr) do { \
    const uint32_t _addr = (uint32_t)(ptr); \
    if ((_addr & 0xFF000000u) == 0x10000000u) { while(1); } \
} while(0)

// NVIC priorities
#define NVIC_PRIO_MOTOR_DMA 1
#define NVIC_PRIO_LED_DMA 3
#define NVIC_PRIO_I2C 5
#define NVIC_PRIO_SPI 5
#define NVIC_PRIO_UART 7
#define NVIC_PRIO_USB 10
#define NVIC_PRIO_SYSTICK 15
