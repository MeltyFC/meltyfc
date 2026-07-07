// MeltyFC — Main Entry Point
// Superloop @2kHz with fresh-data flags. See spec §2.
//
// Execution model:
//   - Phase integration runs EVERY cycle (2kHz)
//   - Sensor reads polled at native ODR (H3LIS 1kHz) via fresh-data flags
//   - DMA drivers (DShot, WS2812) run below the loop
//   - Config CLI processed when USB connected + disarmed

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "stm32f4xx_hal.h"
#endif

#ifdef TARGET_CRUX_F405HD
#include "target.h"
#endif

#include "interfaces.h"
#include "param_registry.h"

// Target tick — 2kHz = 500µs period
static constexpr uint32_t LOOP_PERIOD_US = 500;

// Forward declarations — each module provides init + update
namespace melty {
// These will be implemented as each module is built (P1–P10)
// void sensors_init();
// void sensors_update();
// void heading_init();
// void heading_update(float dt);
// void motors_init();
// void motors_update();
// void crsf_init();
// void crsf_update();
// void safety_init();
// void safety_update();
// void led_init();
// void led_update();
// void config_cli_init();
// void config_cli_update();
}

// --- Platform-agnostic timing helpers ---
#ifndef ARDUINO
static inline uint32_t micros_hal() {
    // HAL_GetTick() returns ms; use DWT cycle counter for µs resolution
    // For now, ms*1000 is sufficient for the skeleton
    return HAL_GetTick() * 1000;
}
#define micros() micros_hal()
#endif

static void melty_setup() {
    // TODO P1: sensor init (I2C scan, H3LIS, ICM)
    // TODO P1: LED init (minimal BOOT/SAFE/ERROR blink codes)
    // TODO P2: DShot init (bidir spike)
    // TODO P3: heading engine init
    // TODO P4: CRSF init, safety init
    // TODO P6: config store load, CLI init

    // Watchdog — fed only by healthy main loop
    // IWatchdog.begin(WATCHDOG_MS * 1000);
}

static void melty_loop() {
    static uint32_t lastLoopUs = 0;
    const uint32_t nowUs = micros();

    // Enforce 2kHz loop rate
    if (nowUs - lastLoopUs < LOOP_PERIOD_US) {
        return;
    }

    const float dt = static_cast<float>(nowUs - lastLoopUs) * 1e-6f;
    lastLoopUs = nowUs;

    // --- Sensor read (fresh-data flagged) ---
    // sensors_update();

    // --- Heading engine (runs every cycle — interpolates between sensor reads) ---
    // heading_update(dt);

    // --- Safety checks (arming, failsafe, watchdog) ---
    // safety_update();

    // --- Motor output (DShot commit) ---
    // motors_update();

    // --- LED (state machine + beacon/POV) ---
    // led_update();

    // --- RC link poll ---
    // crsf_update();

    // --- Config CLI (only when USB connected + safe) ---
    // config_cli_update();

    // --- Feed watchdog ---
    // IWatchdog.reload();
}

#ifdef ARDUINO
void setup() {
    melty_setup();
}
void loop() {
    melty_loop();
}
#else
int main(void) {
    HAL_Init();
    SystemClock_Config(); // Provided by target board bindings
    melty_setup();
    while (1) {
        melty_loop();
    }
}
#endif
