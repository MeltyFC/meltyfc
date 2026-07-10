// MeltyFC — Hardware Abstraction Interfaces
// Core logic includes ONLY this header. Never vendor/target headers.
// New board = new targets/ dir implementing these interfaces. Zero core edits.

#pragma once

#include <cstddef>
#include <cstdint>

namespace melty {

// ============================================================================
// Motor Output
// ============================================================================
struct IMotorOut {
    virtual ~IMotorOut() = default;

    // Send DShot throttle value (0 = disarmed/stop, 48–2047 = throttle range)
    virtual void setThrottle(uint8_t motorIndex, uint16_t value) = 0;

    // Send DShot command (beacon, direction, etc.)
    virtual void sendCommand(uint8_t motorIndex, uint8_t command) = 0;

    // Get bidirectional DShot eRPM telemetry (0 if unavailable)
    virtual uint32_t getErpm(uint8_t motorIndex) const = 0;

    // Returns true if bidir telemetry is available and valid
    virtual bool hasTelemetry(uint8_t motorIndex) const = 0;

    // Commit all throttle values — call once per loop after all setThrottle()
    virtual void update() = 0;

    static constexpr uint8_t MAX_MOTORS = 4;
    static constexpr uint16_t DSHOT_DISARM = 0;
    static constexpr uint16_t DSHOT_MIN = 48;
    static constexpr uint16_t DSHOT_MAX = 2047;
};

// ============================================================================
// High-G Accelerometer (H3LIS331, ±400g)
// ============================================================================
struct IHighGAccel {
    virtual ~IHighGAccel() = default;

    // Read centrifugal axis acceleration in g (the radial axis for spin sensing)
    virtual float readG(uint8_t sensorIndex) const = 0;

    // Raw XYZ for hit detection (sensor frame, in g)
    virtual void readRawG(uint8_t sensorIndex, float& x, float& y, float& z) const = 0;

    // Returns true if sensor is responsive
    virtual bool isHealthy(uint8_t sensorIndex) const = 0;

    // Returns true if reading is >= railed threshold (near full scale)
    virtual bool isRailed(uint8_t sensorIndex) const = 0;

    static constexpr uint8_t SENSOR_INNER = 0;
    static constexpr uint8_t SENSOR_OUTER = 1;
};

// ============================================================================
// IMU (ICM-42688 — gyro Z sign for orientation, accel for low-speed)
// ============================================================================
struct IImu {
    virtual ~IImu() = default;

    // Gyro Z raw value (railed at ±2000dps — magnitude meaningless, SIGN valid)
    virtual float gyroZ() const = 0;

    // Accel magnitude for low-speed omega (±16g range, finer resolution)
    virtual float accelMagnitude() const = 0;

    virtual bool isHealthy() const = 0;
};

// ============================================================================
// RC Link (CRSF/ELRS)
// ============================================================================
struct RcChannels {
    int16_t channels[16]; // Raw CRSF channel values (typically 172–1811)
    uint8_t channelCount;
    bool valid;
};

struct LinkStats {
    int8_t rssi;
    uint8_t lq; // Link quality 0–100%
    int8_t snr;
};

struct IRcLink {
    virtual ~IRcLink() = default;

    // Poll for new data (call each loop)
    virtual void update() = 0;

    // Latest channel data
    virtual RcChannels getChannels() const = 0;

    // Link health
    virtual LinkStats getLinkStats() const = 0;
    virtual bool isLinkUp() const = 0;
    virtual uint32_t msSinceLastFrame() const = 0;

    // Telemetry TX — flight mode text (e.g., "UPRT", "INVT", "SAFE")
    virtual void sendFlightMode(const char* mode) = 0;

    // Telemetry TX — battery voltage
    virtual void sendBatteryVoltage(float volts, float amps = 0.0f) = 0;

    // Telemetry TX — custom sensor (for slip %, hit count, etc.)
    virtual void sendCustomSensor(uint8_t sensorId, float value, const char* label) = 0;
};

// ============================================================================
// LED Strip (WS2812, DMA-driven)
// ============================================================================
struct Color {
    uint8_t r, g, b;

    static Color fromHSV(uint8_t h, uint8_t s, uint8_t v);
    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {r, g, b}; }
    static constexpr Color off() { return {0, 0, 0}; }
};

struct ILedStrip {
    virtual ~ILedStrip() = default;

    virtual void setPixel(uint16_t index, Color color) = 0;
    virtual void setAll(Color color) = 0;
    virtual void clear() = 0;

    // Commit pixel buffer to DMA — non-blocking, returns immediately
    virtual void show() = 0;

    // Is previous DMA transfer still in progress?
    virtual bool isBusy() const = 0;

    virtual uint16_t numPixels() const = 0;
};

// ============================================================================
// Config Store (internal flash, versioned schema)
// ============================================================================
struct IConfigStore {
    virtual ~IConfigStore() = default;

    virtual bool load() = 0;     // Load from flash; returns false if corrupt/missing
    virtual bool save() = 0;     // Persist current params to flash
    virtual void defaults() = 0; // Reset all params to compiled defaults
    virtual uint16_t schemaVersion() const = 0;
};

// ============================================================================
// Flash Storage (SPI flash — POV assets + blackbox ring buffer)
// ============================================================================
struct IFlashStorage {
    virtual ~IFlashStorage() = default;

    // JEDEC probe — returns false if no flash detected
    virtual bool init() = 0;

    virtual bool read(uint32_t addr, uint8_t* buf, size_t len) = 0;

    // R16-5: write() must return within 50µs or report busy via isBusy().
    // Never blocks on program/erase completion. Page programs (0.7–3ms) are
    // started and tracked via isBusy(); the blackbox check-defer layer
    // buffers records during program cycles.
    virtual bool write(uint32_t addr, const uint8_t* buf, size_t len) = 0;
    virtual bool eraseSector(uint32_t addr) = 0;
    virtual bool eraseChip() = 0;

    // R16-5: Returns true if flash is mid-program/erase. Caller must check
    // before write() — blackbox uses blackboxDeferIfBusy() to buffer.
    virtual bool isBusy() const = 0;

    virtual uint32_t capacity() const = 0;   // Total bytes
    virtual uint32_t sectorSize() const = 0; // Erase granularity
};

// ============================================================================
// Audio Beacon (DShot beacon commands — motors ARE the buzzer)
// ============================================================================
struct IAudioBeacon {
    virtual ~IAudioBeacon() = default;

    // Send DShot beacon command (Bluejay motor beep)
    virtual void beep(uint8_t pattern = 1) = 0;
    virtual void stop() = 0;
    virtual bool isBeeping() const = 0;
};

} // namespace melty
