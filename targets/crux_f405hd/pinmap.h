// MeltyFC — CruxF405HD Pin Map
// DERIVED FROM BETAFLIGHT CLI DUMP (docs/PINMAP_source_dump.txt)
//
// !! DO NOT GUESS PINS !!
// All values below are PLACEHOLDERS until the Step 0 dump is captured.
// Trip: connect to Betaflight Configurator, run `dump all` + `resource list`,
// commit to docs/PINMAP_source_dump.txt, then update this file.
//
// Finding 1: This file MUST NOT compile for real hardware until verified.
// Define PINMAP_VERIFIED_FROM_BETAFLIGHT_DUMP after replacing all placeholders.

#pragma once

// Finding 1 gate: When the real dump is captured, remove PINMAP_IS_PLACEHOLDER
// and define PINMAP_VERIFIED_FROM_BETAFLIGHT_DUMP.
// verify.sh warns on placeholder; MUST be resolved before first hardware flash.
#define PINMAP_IS_PLACEHOLDER 1

// ============================================================================
// PLACEHOLDER PIN ASSIGNMENTS — UPDATE FROM STEP 0 DUMP
// ============================================================================

// Motor outputs (DShot300 via timer+DMA)
// Format: {port, pin, timer, channel, dma_stream, dma_channel}
#define MOTOR1_PIN PB0 // PLACEHOLDER — verify from dump
#define MOTOR2_PIN PB1 // PLACEHOLDER
#define MOTOR3_PIN PC9 // PLACEHOLDER
#define MOTOR4_PIN PC8 // PLACEHOLDER (spare)

// Motor timer assignments — MUST match the timer driving each pin
// These determine DMA stream/channel allocation
#define MOTOR_TIMER TIM3 // PLACEHOLDER
#define MOTOR_DMA_STREAM // PLACEHOLDER — from BF resource list

// ICM-42688 IMU (SPI)
#define ICM_SPI SPI1     // PLACEHOLDER
#define ICM_CS_PIN PA4   // PLACEHOLDER
#define ICM_SCK_PIN PA5  // PLACEHOLDER
#define ICM_MISO_PIN PA6 // PLACEHOLDER
#define ICM_MOSI_PIN PA7 // PLACEHOLDER
#define ICM_EXTI_PIN PC4 // PLACEHOLDER (data ready interrupt)

// I2C2 — H3LIS331 high-g accels + (unused) baro
#define I2C_SCL_PIN PB10
#define I2C_SDA_PIN PB11
// H3LIS331 addresses (hardware jumper)
#define H3LIS_ADDR_INNER 0x18 // Default (SDO low)
#define H3LIS_ADDR_OUTER 0x19 // SDO jumpered high

// WS2812 LED strip
#define LED_STRIP_PIN PB6    // PLACEHOLDER — verify from dump
#define LED_STRIP_TIMER TIM4 // PLACEHOLDER — timer driving this pin
#define LED_STRIP_DMA_STREAM // PLACEHOLDER

// UART2 — CRSF (ExpressLRS EP1 receiver)
#define CRSF_UART USART2
#define CRSF_TX_PIN PA2 // PLACEHOLDER
#define CRSF_RX_PIN PA3 // PLACEHOLDER
#define CRSF_BAUD 420000

// Battery voltage sense (ADC)
#define VBAT_PIN                 // PLACEHOLDER — may not be routed
#define VBAT_DIVIDER_RATIO 11.0f // PLACEHOLDER — measure actual divider

// Current sense (ADC) — may not be available
#define CURRENT_PIN // PLACEHOLDER

// SPI flash (blackbox, 8MB)
#define FLASH_SPI    // PLACEHOLDER
#define FLASH_CS_PIN // PLACEHOLDER

// Beeper — Bluejay handles via DShot beacon command, no dedicated pin
// #define BEEPER_PIN           // Not used — see IAudioBeacon

// USB
// STM32F405 uses PA11 (USB_DM) / PA12 (USB_DP) — fixed by silicon

// ============================================================================
// R3-3: CCM RAM guard — DMA buffers MUST NOT reside in CCM
// Call this in every driver init with each DMA buffer pointer.
// ============================================================================
#define ASSERT_NOT_CCM(ptr)                                                                        \
    do {                                                                                           \
        if (((uint32_t)(ptr) & 0xFFFF0000u) == 0x10000000u) {                                      \
            /* DMA buffer in CCM RAM — silent corruption guaranteed */                           \
            while (1)                                                                              \
                ;                                                                                  \
        }                                                                                          \
    } while (0)

// ============================================================================
// R3-6: NVIC Priority Table — set explicitly at init, not left to defaults.
// Lower number = higher priority. F405 has 4 bits = 16 levels.
// ============================================================================
#define NVIC_PRIO_MOTOR_DMA 1 // DShot + bidir turnaround — highest
#define NVIC_PRIO_LED_DMA 3   // WS2812 — high but below motors
#define NVIC_PRIO_I2C 5       // H3LIS331 sensor reads
#define NVIC_PRIO_SPI 5       // ICM-42688 + SPI flash
#define NVIC_PRIO_UART 7      // CRSF
#define NVIC_PRIO_USB 10      // CDC serial — low, never preempts DMA
#define NVIC_PRIO_SYSTICK 15  // Lowest — timing only
