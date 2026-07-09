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
//   2. Clock SCL 9 times (clears any latched slave)
//   3. Generate STOP condition (SDA low→high while SCL high)
//   4. Reconfigure pins for I2C alternate function
//   5. Wire.begin() with timeout
//   6. Probe expected addresses (H3LIS 0x18, 0x19)
//   7. If probe fails, retry from step 1
//
// The GPIO bit-bang (steps 1-4) is target-specific — uses pins from pinmap.h.
// Steps 5-7 use Arduino Wire (portable across families).

bool i2cRecoverAndInit(const I2cRecoveryConfig& cfg = I2C_RECOVERY_DEFAULT);

// Probe a single I2C address — returns true if device ACKs.
// Used after recovery to verify sensors are alive.
bool i2cProbeAddress(uint8_t addr);

} // namespace melty
