// MeltyFC — Dual H3LIS331 High-G Accelerometer Driver (E1)
// I2C, two addresses (0x18 inner, 0x19 outer) on the same bus.
// Provides the heading engine's differential acceleration input.
//
// I2C strategy (decided S4): axis-only 2-byte reads @1kHz, staggered.
// ~260µs per sensor pair per loop iteration.
// Bus recovery: i2c_recovery.hpp (E-01 errata-aware).

#pragma once

#include <cstdint>

namespace melty {

// H3LIS331 register addresses
static constexpr uint8_t H3LIS_REG_WHO_AM_I  = 0x0F;
static constexpr uint8_t H3LIS_REG_CTRL1     = 0x20;
static constexpr uint8_t H3LIS_REG_CTRL4     = 0x23;
static constexpr uint8_t H3LIS_REG_STATUS    = 0x27;
static constexpr uint8_t H3LIS_REG_OUT_X_L   = 0x28;
static constexpr uint8_t H3LIS_REG_OUT_Y_L   = 0x2A;
static constexpr uint8_t H3LIS_REG_OUT_Z_L   = 0x2C;
static constexpr uint8_t H3LIS_WHO_AM_I_VAL  = 0x32;  // Expected WHO_AM_I response

// ±400g range: 12-bit signed, 1 LSB = 0.195g (at ±400g, FS=11)
static constexpr float H3LIS_SCALE_400G = 0.195f;  // g per LSB

struct H3lisReading {
    float x, y, z;  // Acceleration in g
    bool valid;      // False if I2C read failed
};

struct H3lisSensorPair {
    H3lisReading inner;
    H3lisReading outer;
    bool innerPresent;  // WHO_AM_I responded at 0x18
    bool outerPresent;  // WHO_AM_I responded at 0x19
};

// Initialize the I2C bus and both sensors.
// Runs bus recovery (E-01), probes both addresses, configures ±400g / 1kHz ODR.
// Returns true if at least BOTH sensors respond (heading needs differential pair).
bool h3lisInit();

// Read the centripetal axis from both sensors (axis-only, optimized for speed).
// The "centripetal axis" depends on sensor mounting orientation — configurable.
// Returns acceleration in g for each sensor.
H3lisSensorPair h3lisReadPair();

// Read full XYZ from one sensor (diagnostic, slower — 6 bytes vs 2)
H3lisReading h3lisReadXYZ(uint8_t addr);

// Check if sensors are healthy (WHO_AM_I + recent valid reads)
bool h3lisSensorsHealthy();

} // namespace melty
