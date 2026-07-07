// MeltyFC — Stick-Referenced Heading Re-Sync
// PURE LOGIC — computes offset snap from averaged stick angle.
// See spec §9C: hold momentary switch, translation stick becomes pointer,
// release → heading_offset += stick_angle.

#pragma once

#include <cstdint>

namespace melty {

struct ResyncConfig {
    float minDeflection; // Minimum stick deflection to accept (0.0-1.0)
    uint16_t averageMs;  // Average stick angle over this window (ms)
};

struct ResyncState {
    bool held;            // Switch currently held
    float angleAccum;     // Sum of stick angles (for averaging)
    float magAccum;       // Sum of stick magnitudes (for deflection check)
    uint32_t sampleCount; // Number of samples in accumulator
    uint32_t holdStartMs; // When switch was pressed
};

// ============================================================================
// Re-sync functions
// ============================================================================

// Initialize re-sync state
void resyncInit(ResyncState& state);

// Call every loop iteration while re-sync switch state is known.
// switchHeld: CH_RESYNC active
// stickX, stickY: translation stick (-1..+1)
// nowMs: current time
// Returns: offset to ADD to heading_offset on release (0 if cancelled/invalid)
// The return is only meaningful on the transition from held→released.
float resyncUpdate(ResyncState& state, bool switchHeld, float stickX, float stickY, uint32_t nowMs,
                   const ResyncConfig& cfg);

// Compute stick angle from X/Y (radians, 0=forward)
float resyncStickAngle(float stickX, float stickY);

// Compute stick magnitude from X/Y (0.0-1.0)
float resyncStickMagnitude(float stickX, float stickY);

} // namespace melty
