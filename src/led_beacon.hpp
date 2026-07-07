// MeltyFC — LED State Machine + Beacon
// PURE LOGIC — computes LED colors per frame. Hardware driver is separate.
// Priority-stacked state machine per spec §9C.
// See spec §9C (LED state system). P1 minimal, P4 full state machine, P10 POV.

#pragma once

#include <cstddef>
#include <cstdint>

namespace melty {

// ============================================================================
// LED states (priority order — highest first)
// ============================================================================
enum class LedState : uint8_t {
    FAILSAFE,    // Unique red strobe
    ERROR_BLINK, // Count-based blink codes
    LVC_CRIT,    // Red, paired w/ auto spin-down
    CONFIG,      // Config/cal mode
    HIT_FLASH,   // Momentary ring blink on hit
    LVC_WARN,    // Yellow OVERLAY onto current state
    SPINNING,    // Beacon/POV (beacon color = orientation)
    ARMED,       // Unmistakably distinct from SAFE at arena distance
    SAFE,        // Disarmed, zero ESC output — slow dim breathe
    BOOT,        // Self-test sweep
};

// ============================================================================
// RGB color
// ============================================================================
struct RGB {
    uint8_t r, g, b;
};

// ============================================================================
// LED state machine config
// ============================================================================
struct LedConfig {
    uint16_t ledCount;
    float arcWidthDeg; // Beacon arc width in degrees

    RGB beaconColorUp;  // Upright beacon color
    RGB beaconColorInv; // Inverted beacon color
    RGB safeColor;      // Safe/disarmed breathe
    RGB armedColor;     // Armed indicator
    RGB failsafeColor;  // Failsafe strobe
    RGB lvcWarnColor;   // LVC warning overlay
    RGB lvcCritColor;   // LVC critical
};

// ============================================================================
// LED state machine state
// ============================================================================
struct LedStateMachine {
    LedState activeStates[10]; // Bitfield-like: which states are active
    uint8_t activeCount;
    uint8_t errorBlinkCode; // Number of blinks for error state
    uint32_t stateStartMs;  // When the highest-priority state became active
    uint32_t lastUpdateMs;  // Last update timestamp
};

// ============================================================================
// Pure logic functions
// ============================================================================

// Initialize the state machine to BOOT state
void ledSmInit(LedStateMachine& sm, uint32_t nowMs);

// Set/clear a state
void ledSmSetState(LedStateMachine& sm, LedState state, bool active, uint32_t nowMs);

// Set error blink code (also activates ERROR_BLINK state)
void ledSmSetError(LedStateMachine& sm, uint8_t blinkCount, uint32_t nowMs);

// Get the highest-priority active state
LedState ledSmGetActiveState(const LedStateMachine& sm);

// Is LVC_WARN active? (overlay, doesn't replace main state)
bool ledSmIsLvcWarn(const LedStateMachine& sm);

// ============================================================================
// Stationary pattern computation (not spinning)
// ============================================================================

// Compute LED colors for a stationary (non-spinning) frame.
// Fills rgbOut[0..ledCount-1] based on active state + elapsed time.
void ledComputeStationary(const LedStateMachine& sm, const LedConfig& cfg, uint32_t nowMs,
                          RGB* rgbOut);

// ============================================================================
// Spinning pattern computation
// ============================================================================

// Compute LED colors for a spinning frame.
// phase: current bot phase (0..2π)
// headingOffset: trim offset (rad)
// inverted: true if bot is inverted
// Fills rgbOut[0..ledCount-1].
void ledComputeSpinning(const LedStateMachine& sm, const LedConfig& cfg, float phase,
                        float headingOffset, bool inverted, uint32_t nowMs, RGB* rgbOut);

// ============================================================================
// Helper — compute the beacon arc mask for N LEDs
// Returns true if LED at index i should be lit as part of the beacon
// ledAngle: angle this LED occupies in the current revolution (0..2π)
// beaconCenter: the heading direction (phase + headingOffset)
// arcHalfRad: half the arc width in radians
// ============================================================================
bool ledIsInBeaconArc(float ledAngle, float beaconCenter, float arcHalfRad);

// ============================================================================
// Breathe effect — smooth brightness oscillation
// Returns 0.0..1.0 brightness
// ============================================================================
float ledBreathe(uint32_t nowMs, uint32_t periodMs);

// ============================================================================
// Strobe effect — on/off at given rate
// Returns true during the "on" portion
// ============================================================================
bool ledStrobe(uint32_t nowMs, uint32_t periodMs, float dutyCycle);

// ============================================================================
// Blink code — N blinks then pause, repeating
// Returns true during a blink-on period
// ============================================================================
bool ledBlinkCode(uint32_t nowMs, uint8_t blinkCount, uint32_t blinkOnMs, uint32_t blinkOffMs,
                  uint32_t pauseMs);

} // namespace melty
