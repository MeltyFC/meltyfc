// MeltyFC — Low Voltage Cutoff Implementation

#include "lvc.hpp"
#include <cmath>

namespace melty {

uint8_t lvcAutoDetectCells(float packVoltage) {
    // B2/I-31: Detect with AMBIGUITY WINDOWS.
    // Overlap zones where a full N-cell pack equals a discharged (N+1)-cell pack
    // return 0 = AMBIGUOUS → arming refused until explicit cellCount is set.
    //
    // Safe ranges (no overlap):
    //   1S: 2.5–4.35V (no overlap — nothing below 1S)
    //   2S: 5.0–8.7V  (overlap at 4.35–5.0: full 1S vs discharged 2S)
    //   3S: 9.5–13.05V (overlap at 8.7–9.5: full 2S vs discharged 3S)
    //   4S: 13.5–17.4V (overlap at 12.0–13.5: full 3S vs discharged 4S)
    //   5S: 18.0–21.75V
    //   6S: 22.0–26.1V
    if (packVoltage < 2.5f)
        return 0; // Too low
    if (packVoltage <= 4.35f)
        return 1; // 1S: unambiguous
    if (packVoltage < 5.0f)
        return 0; // AMBIGUOUS: full 1S vs discharged 2S
    if (packVoltage <= 8.7f)
        return 2; // 2S: unambiguous
    if (packVoltage < 9.5f)
        return 0; // AMBIGUOUS: full 2S vs discharged 3S
    if (packVoltage <= 13.05f)
        return 3; // 3S: unambiguous
    if (packVoltage < 13.5f)
        return 0; // AMBIGUOUS: full 3S vs discharged 4S
    if (packVoltage <= 17.4f)
        return 4; // 4S: unambiguous
    if (packVoltage < 18.0f)
        return 0; // AMBIGUOUS: full 4S vs discharged 5S
    if (packVoltage <= 21.75f)
        return 5; // 5S: unambiguous
    if (packVoltage < 22.0f)
        return 0; // AMBIGUOUS: full 5S vs discharged 6S
    if (packVoltage <= 26.1f)
        return 6; // 6S: unambiguous
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

float lvcSpinDownThrottle(const LvcState& state, float currentThrottle, uint32_t /*critDurationMs*/) {
    // B5/I-34: DECISION — hard cut, no ramp. Fail-closed beats graceful for a weapon.
    // LVC CRITICAL → choke to zero immediately. No spin-down ramp.
    // The ramp was deleted because a gradual power reduction on a spinning weapon
    // is MORE dangerous than a clean stop (asymmetric torque, loss of control).
    if (state.spinDownActive)
        return 0.0f;
    return currentThrottle;
}

} // namespace melty
