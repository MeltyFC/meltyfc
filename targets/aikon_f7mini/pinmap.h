// MeltyFC — Aikon F7 Mini Pin Map
// DERIVED FROM BETAFLIGHT UNIFIED CONFIG (docs/pinmap_sources/AIKONF7_betaflight_config.h)
// MCU: STM32F722RET6 (F7X2)
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
// BF config: motors on TIM8 (PC6-PC9) — verify timer assignment from BF timer map
// TIMER_PIN_MAP( 0, PC6 , 2,  1)  → TIM8_CH1 (AF3), DMA2_Stream1
// TIMER_PIN_MAP( 1, PC7 , 2,  1)  → TIM8_CH2 (AF3), DMA2_Stream1 (shared? verify)
// TIMER_PIN_MAP( 2, PC8 , 2,  1)  → TIM8_CH3 (AF3), DMA2_Stream1
// TIMER_PIN_MAP( 3, PC9 , 2,  0)  → TIM8_CH4 (AF3), DMA2_Stream0
#define MOTOR1_PIN PC6  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR2_PIN PC7  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR3_PIN PC8  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define MOTOR4_PIN PC9  // BF_CONFIG_DERIVED — verify on live dump at arrival

// Motor timer assignments — from BF timer pin mapping (AF index 2 = TIM8 for PC6-PC9)
#define MOTOR1_TIMER TIM8
#define MOTOR2_TIMER TIM8
#define MOTOR3_TIMER TIM8
#define MOTOR4_TIMER TIM8

// IMU (SPI1) — BF config: GYRO_1_SPI_INSTANCE SPI1
// Supports MPU6000, BMI270, ICM42688P depending on board revision
#define IMU_SPI SPI1      // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_CS_PIN PA4    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_SCK_PIN PA5   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MISO_PIN PA6  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_MOSI_PIN PA7  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define IMU_EXTI_PIN PC4  // BF_CONFIG_DERIVED — verify on live dump at arrival

// I2C2 — H3LIS331 high-g accels + baro
#define I2C_SCL_PIN PB10  // BF_CONFIG_DERIVED — verify on live dump at arrival
#define I2C_SDA_PIN PB11  // BF_CONFIG_DERIVED — verify on live dump at arrival
// H3LIS331 addresses (hardware jumper — MeltyFC-specific, not from BF)
#define H3LIS_ADDR_INNER 0x18 // Default (SDO low)
#define H3LIS_ADDR_OUTER 0x19 // SDO jumpered high

// WS2812 LED strip
// BF config: PA15, TIMER_PIN_MAP( 9, PA15, 1,  0) → TIM2_CH1 (AF1), DMA stream 0
#define LED_STRIP_PIN PA15       // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_TIMER TIM2     // BF_CONFIG_DERIVED — verify on live dump at arrival
#define LED_STRIP_DMA_STREAM 0   // BF_CONFIG_DERIVED — verify on live dump at arrival

// UART2 — CRSF (ExpressLRS receiver)
#define CRSF_UART USART2   // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_TX_PIN PA2    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_RX_PIN PA3    // BF_CONFIG_DERIVED — verify on live dump at arrival
#define CRSF_BAUD 420000

// D1: Battery voltage sense — ADC route tuple
#define VBAT_PIN PC0                 // BF_CONFIG_DERIVED — verify on live dump at arrival
#define VBAT_DIVIDER_RATIO 11.0f     // BF_CONFIG_DERIVED — verify on live dump at arrival
#define VBAT_ADC_INSTANCE ADC1
#define VBAT_ADC_CHANNEL ADC_CHANNEL_10  // PC0 = ADC1_IN10
#define VBAT_GPIO_PORT GPIOC
#define VBAT_GPIO_PIN GPIO_PIN_0
#define VBAT_SAMPLE_TIME ADC_SAMPLETIME_56CYCLES

// Current sense (ADC)
#define CURRENT_PIN PC1              // BF_CONFIG_DERIVED — verify on live dump at arrival

// SPI flash (blackbox) — BF: FLASH_SPI_INSTANCE SPI3, FLASH_CS_PIN PB0
#define FLASH_SPI SPI3               // BF_CONFIG_DERIVED — verify on live dump at arrival
#define FLASH_CS_PIN PB0             // BF_CONFIG_DERIVED — verify on live dump at arrival

// SPI3 bus pins (shared: flash + baro)
#define SPI3_SCK_PIN PB3             // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SPI3_MISO_PIN PB4            // BF_CONFIG_DERIVED — verify on live dump at arrival
#define SPI3_MOSI_PIN PB5            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Baro (SPI3) — BF: BARO_SPI_INSTANCE SPI3, BARO_CS_PIN PB2
#define BARO_CS_PIN PB2              // BF_CONFIG_DERIVED — verify on live dump at arrival

// Beeper — inverted output per BF config
#define BEEPER_PIN PC15              // BF_CONFIG_DERIVED — verify on live dump at arrival
#define BEEPER_INVERTED 1            // BF_CONFIG_DERIVED — verify on live dump at arrival

// Status LED
#define STATUS_LED_PIN PC13          // BF_CONFIG_DERIVED — verify on live dump at arrival

// USB — STM32F722 uses PA11 (USB_DM) / PA12 (USB_DP) — fixed by silicon

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
// A1: Motor Route Table — full timer/channel/AF/DMA tuple per motor
// Derived from BF timer_pin_map + RM0431 DMA stream mapping
// ============================================================================
#define MOTOR1_CHANNEL TIM_CHANNEL_1
#define MOTOR2_CHANNEL TIM_CHANNEL_2
#define MOTOR3_CHANNEL TIM_CHANNEL_3
#define MOTOR4_CHANNEL TIM_CHANNEL_4
#define MOTOR1_AF 3   // AF3 = TIM8
#define MOTOR2_AF 3
#define MOTOR3_AF 3
#define MOTOR4_AF 3
#define MOTOR1_GPIO_PORT GPIOC
#define MOTOR2_GPIO_PORT GPIOC
#define MOTOR3_GPIO_PORT GPIOC
#define MOTOR4_GPIO_PORT GPIOC
#define MOTOR1_GPIO_PIN GPIO_PIN_6
#define MOTOR2_GPIO_PIN GPIO_PIN_7
#define MOTOR3_GPIO_PIN GPIO_PIN_8
#define MOTOR4_GPIO_PIN GPIO_PIN_9
// DMA: TIM8 CH1=DMA2_S2_CH7, CH2=DMA2_S3_CH7, CH3=DMA2_S4_CH7, CH4=DMA2_S7_CH7
#define MOTOR1_DMA_STREAM DMA2_Stream2
#define MOTOR2_DMA_STREAM DMA2_Stream3
#define MOTOR3_DMA_STREAM DMA2_Stream4
#define MOTOR4_DMA_STREAM DMA2_Stream7
#define MOTOR1_DMA_CHANNEL DMA_CHANNEL_7
#define MOTOR2_DMA_CHANNEL DMA_CHANNEL_7
#define MOTOR3_DMA_CHANNEL DMA_CHANNEL_7
#define MOTOR4_DMA_CHANNEL DMA_CHANNEL_7

// LED route
#define LED_STRIP_CHANNEL TIM_CHANNEL_1
#define LED_STRIP_AF 1   // AF1 = TIM2
#define LED_STRIP_GPIO_PORT GPIOA
#define LED_STRIP_GPIO_PIN GPIO_PIN_15
