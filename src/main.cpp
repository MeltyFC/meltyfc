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
// Select HAL header by chip family — one per target, never mixed
#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#else
#error "Unknown chip family — add HAL include for your target"
#endif
#endif

// Every target provides target.h (via -I targets/<board>/)
#if defined(TARGET_CRUX_F405HD) || defined(TARGET_BETAFPV_F411) || \
    defined(TARGET_AIKON_F7MINI) || defined(TARGET_JHEMCU_GHF745) || \
    defined(TARGET_MICOAIR_H743V2) || defined(TARGET_BETAFPV_H725)
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

// --- V-3/C4: Reset cause + A2: Fault breadcrumbs ---
#ifndef ARDUINO
#include "hal/common/reset_cause.hpp"
#include "hal/common/fault_handler_hw.h"
#endif

// --- A3: DWT cycle counter for µs resolution ---
#ifndef ARDUINO
#include "hal/common/dwt_micros.h"
// melty::micros() replaces the HAL_GetTick()*1000 stub
using melty::micros;
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
    static bool firstLoop = true;
    const uint32_t nowUs = micros();

    // Enforce 2kHz loop rate
    if (nowUs - lastLoopUs < LOOP_PERIOD_US) {
        return;
    }

    // D2: First-iteration dt guard — lastLoopUs starts at 0, so first dt
    // would be seconds-since-boot → huge integration step → garbage phase.
    float dt;
    if (firstLoop) {
        dt = static_cast<float>(LOOP_PERIOD_US) * 1e-6f; // Use nominal dt
        firstLoop = false;
    } else {
        dt = static_cast<float>(nowUs - lastLoopUs) * 1e-6f;
        // Clamp dt to prevent integration blowup after stalls
        if (dt > 0.01f)
            dt = static_cast<float>(LOOP_PERIOD_US) * 1e-6f;
    }
    lastLoopUs = nowUs;

    // --- Sensor read (fresh-data flagged) ---
    // sensors_update();

    // --- Heading engine (runs every cycle — interpolates between sensor reads) ---
    // heading_update(dt);

    // --- Safety checks (arming, failsafe, watchdog) ---
    // safety_update();
    //
    // D3 BINDING CONTRACT: LVC_CRIT → armState forced to DISARMED via
    // ArmPreconditions.sensorsHealthy or a dedicated lvcCritical flag.
    // The LVC module's throttle ramp-down runs in parallel, but the
    // safety machine is the backstop — if LVC says CRITICAL, motors die.
    //
    // D4 BINDING CONTRACT: creepComputeOutput() results must be passed
    // through chokeMotorOutput(output, armState) before reaching DShot.
    // Creep mode is NOT an authority bypass — the choke point holds.

    // --- Motor output (DShot commit) ---
    // For each motor i:
    //   float raw = (creepActive) ? creepOut[i] : meltyMixOut[i];
    //   float safe = chokeMotorOutput(raw, armState);
    //   uint16_t dshot = throttleToDshot(safe, armState);
    //   dshotWrite(i, dshot);
    // motors_update();

    // --- LED (state machine + beacon/POV) ---
    // led_update();

    // --- RC link poll ---
    // crsf_update();

    // --- USB debug streaming (S3: guarded writes) ---
    // if (debugStreamEnabled && Serial && Serial.availableForWrite() >= 64) {
    //     // Only write if buffer has room — never block the loop
    //     Serial.printf("%.1f,%.1f,%d\n", omegaToRpm(omega), phase, (int)armState);
    // }

    // --- Config CLI (only when USB connected + safe) ---
    // S3 contract: ALL Serial writes in the CLI handler are guarded:
    //   if (!Serial || Serial.availableForWrite() < len) return;
    // Default: debug streaming OFF, enabled via `stream on` CLI command.
    // config_cli_update();

    // --- Feed watchdog ---
    // R3-4 contract: IWDG started AFTER init, BEFORE arming is reachable.
    // Arming gated on watchdog-running.
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

    // I-12: Clock assert — refuse to arm if clock is wrong
    // Mismatch = clock config failed or HSE didn't start
    #if defined(TARGET_CLOCK_MHZ)
    {
        uint32_t expectedHz = TARGET_CLOCK_MHZ * 1000000UL;
        uint32_t tolerance  = expectedHz / 20; // 5% tolerance
        if (SystemCoreClock < (expectedHz - tolerance) ||
            SystemCoreClock > (expectedHz + tolerance)) {
            // Clock mismatch — blink error code, do NOT proceed
            // Motors will never arm (melty_setup not called)
            while (1) {
                // TODO: ERROR blink code (fast red flash)
                // IWDG will reset eventually if enabled
            }
        }
    }
    #endif

    // V-3/C4: Reset cause forensics — read + clear RCC_CSR FIRST
    // Pairs with A2 fault breadcrumbs for full post-mortem
    melty::ResetInfo resetInfo = melty::readResetCause();
    (void)resetInfo; // Used by CLI status + blackbox at integration

    // A3: DWT cycle counter — must be after SystemClock_Config
    melty::dwtInit();

    // V-3/A2: Check fault breadcrumbs from previous crash
    // Available for blackbox header + CLI `status` display
    if (melty::faultBreadcrumbValid()) {
        // Previous run crashed — breadcrumbs contain PC/LR/CFSR/HFSR
        // Paired with resetInfo for complete post-mortem story
        // CLI will display on next `status` command; blackbox logs on init
        // Don't clear until after logging — cleared by CLI `fault clear` command
    }

    melty_setup();
    while (1) {
        melty_loop();
    }
}
#endif
