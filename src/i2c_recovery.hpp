// MeltyFC — I2C Bus Recovery
// R7-2 / R3-4: Bounded I2C init with bus recovery.
//
// Problem: If an I2C slave (H3LIS331) is latched (SDA held low from a
// mid-transfer reset), Wire.begin() hangs indefinitely → boot hang.
// This violates I-6 (bounded init, ERROR-not-hang).
//
// Fix: Before Wire.begin(), bit-bang 9 SCL clocks + STOP condition.
// This clears any latched slave state. Then Wire.begin() with timeout.
// If bus still fails after recovery → return false, let main() enter
// ERROR state with blink code (not hang).
//
// Pure logic interface — GPIO bit-bang implementation is per-family.

#pragma once

#include <cstdint>

namespace melty {

struct I2cRecoveryConfig {
    uint8_t maxRetries;        // How many times to attempt recovery (default 3)
    uint16_t sclPulseDelayUs;  // Microseconds per SCL half-cycle (default 5)
    uint32_t initTimeoutMs;    // Wire.begin + first read must complete in this time
};

static constexpr I2cRecoveryConfig I2C_RECOVERY_DEFAULT = {
    .maxRetries = 3,
    .sclPulseDelayUs = 5,
    .initTimeoutMs = 100,
};

// Attempt I2C bus recovery and initialization.
// Returns true if bus is healthy and sensors responding.
// Returns false if recovery failed after maxRetries — caller should enter ERROR state.
//
// Implementation:
//   1. Configure SCL/SDA as GPIO outputs
//   2. Clock SCL 9 times (clears any latched SLAVE — the common case)
//   3. Generate STOP condition (SDA low->high while SCL high)
//   4. E-01: Force-reset I2C MASTER peripheral via RCC
//      __HAL_RCC_I2Cx_FORCE_RESET() -> short delay -> RELEASE_RESET()
//      This clears the F4 analog-filter BUSY lockup erratum (ES0182)
//      where the MASTER wedges and re-init alone doesn't clear it.
//      Three lines, ST's own documented workaround.
//   4.5: ES0182 §2.9.6 / ES0392 §2.19.4: Clear spurious BERR flag
//      The I2C peripheral can set BERR incorrectly in master mode.
//      Do NOT abort transfer on BERR alone — clear the flag and continue.
//      After RCC reset (step 4), clear SR1.BERR (F4) or ISR.BERR (F7/H7)
//      before re-init.
//   5. Reconfigure pins for I2C alternate function
//   6. Wire.begin() / HAL_I2C_Init() with timeout
//   7. Probe expected addresses (H3LIS 0x18, 0x19)
//   8. If probe fails, retry from step 1
//
// With both halves (slave-side SCL + master-side RCC reset), the module
// handles the COMPLETE wedge set. The GPIO bit-bang (steps 1-3) is
// target-specific. The RCC reset (step 4) is family-portable.
// Steps 6-8 use HAL I2C (stm32cube framework).

bool i2cRecoverAndInit(const I2cRecoveryConfig& cfg = I2C_RECOVERY_DEFAULT);

// Probe a single I2C address — returns true if device ACKs.
// Used after recovery to verify sensors are alive.
bool i2cProbeAddress(uint8_t addr);

} // namespace melty
