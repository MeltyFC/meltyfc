// MeltyFC — LED State Machine + Beacon Implementation
// PURE LOGIC — no hardware. Computes colors per frame.

#include "led_beacon.hpp"
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace melty {

// ============================================================================
// State machine
// ============================================================================
void ledSmInit(LedStateMachine& sm, uint32_t nowMs) {
    memset(&sm, 0, sizeof(sm));
    sm.activeStates[0] = LedState::BOOT;
    sm.activeCount = 1;
    sm.stateStartMs = nowMs;
    sm.lastUpdateMs = nowMs;
}

static bool hasState(const LedStateMachine& sm, LedState state) {
    for (uint8_t i = 0; i < sm.activeCount; i++) {
        if (sm.activeStates[i] == state) return true;
    }
    return false;
}

void ledSmSetState(LedStateMachine& sm, LedState state, bool active, uint32_t nowMs) {
    if (active) {
        if (!hasState(sm, state) && sm.activeCount < 10) {
            sm.activeStates[sm.activeCount++] = state;
        }
    } else {
        for (uint8_t i = 0; i < sm.activeCount; i++) {
            if (sm.activeStates[i] == state) {
                // Remove by shifting
                for (uint8_t j = i; j < sm.activeCount - 1; j++) {
                    sm.activeStates[j] = sm.activeStates[j + 1];
                }
                sm.activeCount--;
                break;
            }
        }
    }
    sm.lastUpdateMs = nowMs;
}

void ledSmSetError(LedStateMachine& sm, uint8_t blinkCount, uint32_t nowMs) {
    sm.errorBlinkCode = blinkCount;
    ledSmSetState(sm, LedState::ERROR_BLINK, true, nowMs);
}

LedState ledSmGetActiveState(const LedStateMachine& sm) {
    // Return highest priority (lowest enum value) active state
    LedState best = LedState::BOOT;  // Lowest priority
    bool found = false;
    for (uint8_t i = 0; i < sm.activeCount; i++) {
        if (!found || static_cast<uint8_t>(sm.activeStates[i]) < static_cast<uint8_t>(best)) {
            best = sm.activeStates[i];
            found = true;
        }
    }
    return found ? best : LedState::SAFE;
}

bool ledSmIsLvcWarn(const LedStateMachine& sm) {
    return hasState(sm, LedState::LVC_WARN);
}

// ============================================================================
// Effects
// ============================================================================
float ledBreathe(uint32_t nowMs, uint32_t periodMs) {
    if (periodMs == 0) return 0.5f;
    float t = static_cast<float>(nowMs % periodMs) / static_cast<float>(periodMs);
    // Sine wave: 0→1→0 over one period
    return 0.5f * (1.0f + sinf(t * 2.0f * M_PI));
}

bool ledStrobe(uint32_t nowMs, uint32_t periodMs, float dutyCycle) {
    if (periodMs == 0) return true;
    float t = static_cast<float>(nowMs % periodMs) / static_cast<float>(periodMs);
    return t < dutyCycle;
}

bool ledBlinkCode(uint32_t nowMs, uint8_t blinkCount, uint32_t blinkOnMs,
                  uint32_t blinkOffMs, uint32_t pauseMs) {
    if (blinkCount == 0) return false;
    uint32_t cycleLen = (blinkOnMs + blinkOffMs) * blinkCount + pauseMs;
    uint32_t pos = nowMs % cycleLen;

    // Which blink are we in?
    for (uint8_t i = 0; i < blinkCount; i++) {
        uint32_t start = i * (blinkOnMs + blinkOffMs);
        if (pos >= start && pos < start + blinkOnMs) return true;
    }
    return false;
}

// ============================================================================
// Beacon arc
// ============================================================================
bool ledIsInBeaconArc(float ledAngle, float beaconCenter, float arcHalfRad) {
    // Normalize angles to [0, 2π)
    float diff = ledAngle - beaconCenter;
    // Wrap to [-π, π)
    while (diff > M_PI) diff -= 2.0f * M_PI;
    while (diff < -M_PI) diff += 2.0f * M_PI;
    return fabsf(diff) <= arcHalfRad;
}

// ============================================================================
// Helper — scale RGB by brightness
// ============================================================================
static RGB scaleRgb(RGB c, float brightness) {
    return {
        static_cast<uint8_t>(c.r * brightness),
        static_cast<uint8_t>(c.g * brightness),
        static_cast<uint8_t>(c.b * brightness)
    };
}

// ============================================================================
// Apply LVC warning overlay (yellow tint blended onto existing color)
// ============================================================================
static RGB applyLvcOverlay(RGB base, RGB warnColor, uint32_t nowMs) {
    bool flash = ledStrobe(nowMs, 500, 0.5f);
    if (!flash) return base;
    // Blend: average of base and warning color
    return {
        static_cast<uint8_t>((base.r + warnColor.r) / 2),
        static_cast<uint8_t>((base.g + warnColor.g) / 2),
        static_cast<uint8_t>((base.b + warnColor.b) / 2)
    };
}

// ============================================================================
// Stationary pattern
// ============================================================================
void ledComputeStationary(const LedStateMachine& sm, const LedConfig& cfg,
                          uint32_t nowMs, RGB* rgbOut) {
    LedState state = ledSmGetActiveState(sm);
    bool lvcWarn = ledSmIsLvcWarn(sm);

    for (uint16_t i = 0; i < cfg.ledCount; i++) {
        RGB color = {0, 0, 0};

        switch (state) {
            case LedState::BOOT: {
                // Sweep: one LED at a time cycling through
                uint16_t pos = (nowMs / 50) % cfg.ledCount;
                color = (i == pos) ? RGB{0, 128, 255} : RGB{0, 0, 0};
                break;
            }
            case LedState::SAFE: {
                // Slow dim breathe
                float b = ledBreathe(nowMs, 3000);
                color = scaleRgb(cfg.safeColor, b);
                break;
            }
            case LedState::ARMED: {
                // Solid color, pulsing faster than SAFE
                float b = 0.5f + 0.5f * ledBreathe(nowMs, 1000);
                color = scaleRgb(cfg.armedColor, b);
                break;
            }
            case LedState::CONFIG: {
                // Alternating colors
                bool phase = (i % 2 == 0) ^ ledStrobe(nowMs, 1000, 0.5f);
                color = phase ? RGB{0, 0, 255} : RGB{255, 128, 0};
                break;
            }
            case LedState::FAILSAFE: {
                // Fast red strobe — all LEDs
                bool on = ledStrobe(nowMs, 200, 0.5f);
                color = on ? cfg.failsafeColor : RGB{0, 0, 0};
                break;
            }
            case LedState::ERROR_BLINK: {
                // Blink code — all LEDs flash N times
                bool on = ledBlinkCode(nowMs, sm.errorBlinkCode, 200, 200, 1000);
                color = on ? RGB{255, 0, 0} : RGB{0, 0, 0};
                break;
            }
            case LedState::LVC_CRIT: {
                // Red, all LEDs, fast pulse
                float b = 0.5f + 0.5f * ledBreathe(nowMs, 500);
                color = scaleRgb(cfg.lvcCritColor, b);
                break;
            }
            case LedState::HIT_FLASH: {
                // White flash, all LEDs
                color = {255, 255, 255};
                break;
            }
            default:
                break;
        }

        // Apply LVC warning overlay if active (doesn't replace, blends)
        if (lvcWarn && state != LedState::FAILSAFE && state != LedState::LVC_CRIT) {
            color = applyLvcOverlay(color, cfg.lvcWarnColor, nowMs);
        }

        rgbOut[i] = color;
    }
}

// ============================================================================
// Spinning pattern
// ============================================================================
void ledComputeSpinning(const LedStateMachine& sm, const LedConfig& cfg,
                        float phase, float headingOffset, bool inverted,
                        uint32_t nowMs, RGB* rgbOut) {
    LedState state = ledSmGetActiveState(sm);
    bool lvcWarn = ledSmIsLvcWarn(sm);

    // If failsafe/error/lvc_crit, override spinning with stationary pattern
    if (state == LedState::FAILSAFE || state == LedState::ERROR_BLINK ||
        state == LedState::LVC_CRIT) {
        ledComputeStationary(sm, cfg, nowMs, rgbOut);
        return;
    }

    float beaconCenter = phase + headingOffset;
    float arcHalfRad = (cfg.arcWidthDeg / 2.0f) * (M_PI / 180.0f);
    RGB beaconColor = inverted ? cfg.beaconColorInv : cfg.beaconColorUp;

    for (uint16_t i = 0; i < cfg.ledCount; i++) {
        RGB color = {0, 0, 0};

        // Each LED has a fixed angular position on the bot
        // LED 0 = 0°, LED N = N * (360/ledCount)°
        float ledAngle = phase + static_cast<float>(i) * (2.0f * M_PI / static_cast<float>(cfg.ledCount));

        if (ledIsInBeaconArc(ledAngle, beaconCenter, arcHalfRad)) {
            color = beaconColor;
        }

        // Hit flash overlay
        if (state == LedState::HIT_FLASH) {
            color = {255, 255, 255};
        }

        // LVC warning overlay
        if (lvcWarn) {
            color = applyLvcOverlay(color, cfg.lvcWarnColor, nowMs);
        }

        rgbOut[i] = color;
    }
}

} // namespace melty
