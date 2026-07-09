// MeltyFC — JHEMCU GHF745 AIO Pin Map
// DERIVED FROM BETAFLIGHT UNIFIED CONFIG (docs/pinmap_sources/JHEF745_betaflight_config.h)
// MCU: STM32F745VGT6 (F745)
//
// All pin assignments below are derived from the Betaflight unified-target config.
// They are one grade stronger than guessing, one grade weaker than a live dump.
// Step 0 on real hardware: `dump all` + `resource list` → diff against this file.
//
// Finding 1 gate: define PINMAP_VERIFIED_FROM_BETAFLIGHT_DUMP after live verification.
// verify.sh warns on BF_CONFIG_DERIVED; MUST be resolved before first hardware flash.

#pragma once

#define PINMAP_IS_BF_CONFIG_DERIVED 1

// ============================================================================
// BF_CONFIG_DERIVED PIN ASSIGNMENTS — VERIFY ON LIVE DUMP AT ARRIVAL
// ============================================================================

// Motor outputs (DShot300 via timer+DMA)
// BF config: motors 1-2 on TIM3 (PB0/PB1), motors 3-4 on TIM1 (PE9/PE11)
// TIMER_PIN_MAP( 1, PB0 , 2,  0)  → TIM3_CH3 (AF2), DMA stream 0
// TIMER_PIN_MAP( 2, PB1 , 2,  0)  → TIM3_CH4 (AF2), DMA stream 0
// TIMER_PIN_MAP( 3, PE9 , 1,  0)  → TIM1_CH1 (AF1), DMA stream 0
// TIMER_PIN_MAP( 4, PE11, 1,  1)  → TIM1_CH2 (AF1), DMA stream 1
#define MOTOR1_PIN PB0   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR2_PIN PB1   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR3_PIN PE9   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR4_PIN PE11  // BF_CONFIG_DERIVED — verify on live dump at arrival

// Motor timer assignments — split across TIM3 (M1/M2) and TIM1 (M3/M4)
#define MOTOR1_TIMER TIM3 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR2_TIMER TIM3 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR3_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR4_TIMER TIM1 // BF_CONFIG_DERIVED — verify on live dump at arrival

// IMU (SPI4) — BF config: GYRO_1_SPI_INSTANCE SPI4, GYRO_1_ALIGN CW270_DEG
#define IMU_SPI SPI4      // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_CS_PIN PE4    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_SCK_PIN PE2   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MISO_PIN PE5  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MOSI_PIN PE6  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_EXTI_PIN PE1  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_ALIGN_CW270 1 // BF_CONFIG_DERIVED — verify on live dump at arrival

// I2C1 — H3LIS331 high-g accels + baro (BMP280/DPS310)
#define I2C_SCL_PIN PB6   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define I2C_SDA_PIN PB7   // BF_CONFIG_DERIVED — verify on live dump at arrival
// H3LIS331 addresses (hardware jumper — MeltyFC-specific, not from BF)
#define H3LIS_ADDR_INNER 0x18 // Default (SDO low)
#define H3LIS_ADDR_OUTER 0x19 // SDO jumpered high

// WS2812 LED strip
// BF config: PD12, TIMER_PIN_MAP( 7, PD12, 1,  0) → TIM4_CH1 (AF2), DMA stream 0
#define LED_STRIP_PIN PD12       // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_TIMER TIM4     // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_DMA_STREAM 0   // BF_CONFIG_DERIVED — verify on live dump at arrival

// UART6 — CRSF (ExpressLRS receiver) — using UART6 as common RX port
#define CRSF_UART USART6   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_TX_PIN PC6    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_RX_PIN PC7    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_BAUD 420000

// Battery voltage sense (ADC)
#define VBAT_PIN PC3                 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define VBAT_DIVIDER_RATIO 11.0f     // BF_CONFIG_DERIVED — verify on live dump at arrival

// Current sense (ADC) — BF: DEFAULT_CURRENT_METER_SCALE 275
#define CURRENT_PIN PC2              // BF_CONFIG_DERIVED — verify on live dump at arrival

// SPI flash (blackbox) — BF: FLASH_SPI_INSTANCE SPI1, FLASH_CS_PIN PA4
// Supports M25P16 and PY25Q128HA
#define FLASH_SPI SPI1               // BF_CONFIG_DERIVED — verify on live dump at arrival
#define FLASH_CS_PIN PA4             // BF_CONFIG_DERIVED — verify on live dump at arrival

// SPI1 bus pins (shared: flash)
#define SPI1_SCK_PIN PA5             // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SPI1_MISO_PIN PA6            // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SPI1_MOSI_PIN PA7            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Beeper — inverted output per BF config
#define BEEPER_PIN PD15              // BF_CONFIG_DERIVED — verify on live dump at arrival
#define BEEPER_INVERTED 1            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Status LED
#define STATUS_LED_PIN PA2           // BF_CONFIG_DERIVED — verify on live dump at arrival

// USB detect
#define USB_DETECT_PIN PA8           // BF_CONFIG_DERIVED — verify on live dump at arrival

// USB — STM32F745 uses PA11 (USB_DM) / PA12 (USB_DP) — fixed by silicon

// ============================================================================
// I-11a: DTCM RAM guard — DMA buffers MUST reside in DTCM on F7
// F7 DTCM (64KB at 0x20000000-0x2000FFFF) is never cached AND is
// DMA-accessible via AHBS slave port. This solves DMA coherency for free.
// Call this in every driver init with each DMA buffer pointer.
// ============================================================================
#define ASSERT_IN_DTCM(ptr)                                                                            \
    do {                                                                                               \
        const uint32_t _addr = (uint32_t)(ptr);                                                        \
        if (_addr < 0x20000000u || _addr > 0x2000FFFFu) {                                              \
            /* DMA buffer NOT in DTCM — cache-coherency failure guaranteed */                        \
            while (1)                                                                                  \
                ;                                                                                      \
        }                                                                                              \
    } while (0)

// ============================================================================
// R3-6: NVIC Priority Table — set explicitly at init, not left to defaults.
// Lower number = higher priority. F7 has 4 bits = 16 levels.
// ============================================================================
#define NVIC_PRIO_MOTOR_DMA 1 // DShot + bidir turnaround — highest
#define NVIC_PRIO_LED_DMA 3   // WS2812 — high but below motors
#define NVIC_PRIO_I2C 5       // H3LIS331 sensor reads
#define NVIC_PRIO_SPI 5       // IMU + SPI flash
#define NVIC_PRIO_UART 7      // CRSF
#define NVIC_PRIO_USB 10      // CDC serial — low, never preempts DMA
#define NVIC_PRIO_SYSTICK 15  // Lowest — timing only

// ============================================================================
// A1: Motor Route Table
// ============================================================================
#define MOTOR1_CHANNEL TIM_CHANNEL_3
#define MOTOR2_CHANNEL TIM_CHANNEL_4
#define MOTOR3_CHANNEL TIM_CHANNEL_1
#define MOTOR4_CHANNEL TIM_CHANNEL_2
#define MOTOR1_AF 2   // AF2 = TIM3
#define MOTOR2_AF 2
#define MOTOR3_AF 1   // AF1 = TIM1
#define MOTOR4_AF 1
#define MOTOR1_GPIO_PORT GPIOB
#define MOTOR2_GPIO_PORT GPIOB
#define MOTOR3_GPIO_PORT GPIOE
#define MOTOR4_GPIO_PORT GPIOE
#define MOTOR1_GPIO_PIN GPIO_PIN_0
#define MOTOR2_GPIO_PIN GPIO_PIN_1
#define MOTOR3_GPIO_PIN GPIO_PIN_9
#define MOTOR4_GPIO_PIN GPIO_PIN_11
#define MOTOR1_DMA_STREAM DMA1_Stream7
#define MOTOR2_DMA_STREAM DMA1_Stream2
#define MOTOR3_DMA_STREAM DMA2_Stream1
#define MOTOR4_DMA_STREAM DMA2_Stream2
#define MOTOR1_DMA_CHANNEL DMA_CHANNEL_5
#define MOTOR2_DMA_CHANNEL DMA_CHANNEL_5
#define MOTOR3_DMA_CHANNEL DMA_CHANNEL_6
#define MOTOR4_DMA_CHANNEL DMA_CHANNEL_6

#define LED_STRIP_CHANNEL TIM_CHANNEL_1
#define LED_STRIP_AF 2
#define LED_STRIP_GPIO_PORT GPIOD
#define LED_STRIP_GPIO_PIN GPIO_PIN_12
