// MeltyFC — Low Voltage Cutoff Implementation

#include "lvc.hpp"
#include <cmath>

namespace melty {

uint8_t lvcAutoDetectCells(float packVoltage) {
    // Detect based on voltage ranges at typical charge levels
    // Fully charged: ~4.2V/cell. Storage: ~3.8V. Cutoff: ~3.0V.
    // Allow up to 4.35V/cell for HV packs.
    if (packVoltage < 2.5f)
        return 0; // Too low to detect
    if (packVoltage <= 4.35f)
        return 1; // 1S: 2.5–4.35V
    if (packVoltage <= 8.7f)
        return 2; // 2S: 4.36–8.7V
    if (packVoltage <= 13.05f)
        return 3; // 3S: 8.71–13.05V
    if (packVoltage <= 17.4f)
        return 4; // 4S: 13.06–17.4V
    if (packVoltage <= 21.75f)
        return 5; // 5S: 17.41–21.75V
    if (packVoltage <= 26.1f)
        return 6; // 6S: 21.76–26.1V
    return 0;     // Out of range
}

LvcLevel lvcUpdate(LvcState& state, float packVoltage, const LvcConfig& cfg) {
    // DI-16: Reject NaN/Inf voltage as sensor fault
    if (!std::isfinite(packVoltage) || packVoltage < 0.0f) {
        state.packVoltage = 0.0f;
        state.cellVoltage = 0.0f;
        state.level = LvcLevel::SENSOR_FAULT;
        return LvcLevel::SENSOR_FAULT;
    }

    state.packVoltage = packVoltage;

    // Determine cell count
    if (cfg.cellCount > 0) {
        state.detectedCells = cfg.cellCount;
    } else {
        if (state.detectedCells == 0) {
            state.detectedCells = lvcAutoDetectCells(packVoltage);
        }
        // Don't re-detect once set (prevents flapping as voltage drops)
    }

    if (state.detectedCells == 0) {
        // Finding 3: Fail CLOSED — unknown cell count = no motor authority
        state.cellVoltage = 0.0f;
        if (packVoltage < 2.5f) {
            // No battery detected (USB-only or disconnected ADC)
            state.level = LvcLevel::SENSOR_FAULT;
        } else {
            // Battery present but cell count ambiguous
            state.level = LvcLevel::CELL_COUNT_UNKNOWN;
        }
        return state.level;
    }

    state.cellVoltage = packVoltage / static_cast<float>(state.detectedCells);

    // R7-6: Hysteresis band on recovery edge.
    // Drop into WARN/CRITICAL at threshold. Recover only above threshold + hysteresis.
    // Prevents LED/telemetry chatter at boundary. CRITICAL latch is independent.
    float hyst = cfg.hysteresisVolts;

    if (state.cellVoltage <= cfg.critVolts) {
        state.level = LvcLevel::CRITICAL;
        state.spinDownActive = true;
    } else if (state.cellVoltage <= cfg.warnVolts) {
        state.level = LvcLevel::WARN;
        // Don't clear spinDownActive — once triggered, stays until re-arm
    } else if (state.level == LvcLevel::WARN &&
               state.cellVoltage < cfg.warnVolts + hyst) {
        // In hysteresis band — stay in WARN until above warn + hysteresis
        state.level = LvcLevel::WARN;
    } else {
        state.level = LvcLevel::OK;
    }

    return state.level;
}

float lvcSpinDownThrottle(const LvcState& state, float currentThrottle, uint32_t critDurationMs) {
    if (!state.spinDownActive)
        return currentThrottle;

    // Ramp down over 2000ms
    if (critDurationMs >= 2000)
        return 0.0f;

    float ramp = 1.0f - (static_cast<float>(critDurationMs) / 2000.0f);
    return currentThrottle * ramp;
}

} // namespace melty
