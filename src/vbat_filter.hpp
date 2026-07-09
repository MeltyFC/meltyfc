// MeltyFC — VBAT Filter: Sag Filtering + Arming Transient Hold-off
// Pure logic — no hardware dependencies. Testable natively.
//
// R7-1: Raw ADC voltage is noisy (motor inrush, ESR sag, ripple).
// Without filtering:
//   - Motor spin-up drops pack 0.4V → false LVC CRITICAL mid-fight
//   - Ripple noise triggers warn/critical oscillation
//   - Arming transient (capacitor charge) reads low → blocks arm
//
// Filter: EMA (exponential moving average) + arming grace window.
// EMA smooths ripple; grace window ignores sag during spin-up.

#pragma once

#include <cstdint>

namespace melty {

struct VbatFilterConfig {
    uint16_t emaSmoothingNum;      // EMA numerator (e.g., 7 for 7/8 weight on old)
    uint16_t emaSmoothingDen;      // EMA denominator (e.g., 8)
    uint32_t armingGraceMs;        // Ignore sag for this long after arming (spin-up inrush)
    uint16_t minValidMv;           // Below this = ADC fault, not real voltage
    uint16_t maxValidMv;           // Above this = ADC fault
};

// Default config: 7/8 EMA (~8-sample time constant), 500ms arming grace,
// valid range 2000-26000mV (covers 1S through 6S)
static constexpr VbatFilterConfig VBAT_FILTER_DEFAULT = {
    .emaSmoothingNum = 7,
    .emaSmoothingDen = 8,
    .armingGraceMs = 500,
    .minValidMv = 2000,
    .maxValidMv = 26000,
};

class VbatFilter {
public:
    void init(const VbatFilterConfig& cfg = VBAT_FILTER_DEFAULT) {
        cfg_ = cfg;
        filteredMv_ = 0;
        initialized_ = false;
        valid_ = false;
        armTimeMs_ = 0;
        inGrace_ = false;
    }

    // Feed a new raw millivolt reading. Call at ADC sample rate (~1kHz typical).
    // Returns the filtered voltage in mV.
    uint16_t update(uint16_t rawMv, uint32_t nowMs) {
        // Range check
        if (rawMv < cfg_.minValidMv || rawMv > cfg_.maxValidMv) {
            valid_ = false;
            return filteredMv_;
        }

        valid_ = true;

        if (!initialized_) {
            // First valid sample — seed the filter
            filteredMv_ = rawMv;
            initialized_ = true;
            return filteredMv_;
        }

        // EMA: filtered = (num * old + (den - num) * new) / den
        uint32_t weighted = (uint32_t)cfg_.emaSmoothingNum * filteredMv_ +
                            (uint32_t)(cfg_.emaSmoothingDen - cfg_.emaSmoothingNum) * rawMv;
        filteredMv_ = (uint16_t)(weighted / cfg_.emaSmoothingDen);

        // During arming grace window, clamp filtered voltage to prevent false sag
        if (inGrace_ && (nowMs - armTimeMs_) < cfg_.armingGraceMs) {
            // Don't let filtered voltage drop below pre-arm level during grace
            if (filteredMv_ < preArmMv_) {
                filteredMv_ = preArmMv_;
            }
        } else {
            inGrace_ = false;
        }

        return filteredMv_;
    }

    // Call when arming — starts the grace window
    void onArm(uint32_t nowMs) {
        armTimeMs_ = nowMs;
        preArmMv_ = filteredMv_;
        inGrace_ = true;
    }

    // Call when disarming — ends grace window
    void onDisarm() {
        inGrace_ = false;
    }

    uint16_t filteredMv() const { return filteredMv_; }
    bool isValid() const { return valid_ && initialized_; }
    bool isInitialized() const { return initialized_; }

private:
    VbatFilterConfig cfg_;
    uint16_t filteredMv_;
    uint16_t preArmMv_;
    uint32_t armTimeMs_;
    bool initialized_;
    bool valid_;
    bool inGrace_;
};

} // namespace melty
