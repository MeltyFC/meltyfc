// MeltyFC — Dual H3LIS331 Driver Implementation (E1)
// I2C bus driver with E-01 errata-aware recovery.

#ifndef NATIVE_BUILD

#include "sensors_h3lis.hpp"
#include "i2c_recovery.hpp"

// Use STM32 HAL I2C (stm32cube framework)
#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif

#include "target.h"

namespace melty {

static I2C_HandleTypeDef hi2c;
static bool initialized = false;
static uint8_t innerAddr = H3LIS_ADDR_INNER;
static uint8_t outerAddr = H3LIS_ADDR_OUTER;

// I2C timeout for single transaction (ms)
static constexpr uint32_t I2C_TIMEOUT_MS = 5;

// --- Low-level I2C helpers ---

static bool i2cWriteReg(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t data[2] = { reg, val };
    return HAL_I2C_Master_Transmit(&hi2c, addr << 1, data, 2, I2C_TIMEOUT_MS) == HAL_OK;
}

static bool i2cReadReg(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    // Set MSB of reg for auto-increment (multi-byte read)
    uint8_t regAutoInc = reg | 0x80;
    if (HAL_I2C_Master_Transmit(&hi2c, addr << 1, &regAutoInc, 1, I2C_TIMEOUT_MS) != HAL_OK)
        return false;
    return HAL_I2C_Master_Receive(&hi2c, addr << 1, buf, len, I2C_TIMEOUT_MS) == HAL_OK;
}

static bool i2cProbe(uint8_t addr) {
    return HAL_I2C_IsDeviceReady(&hi2c, addr << 1, 2, I2C_TIMEOUT_MS) == HAL_OK;
}

static bool configureSensor(uint8_t addr) {
    // Check WHO_AM_I
    uint8_t whoAmI = 0;
    if (!i2cReadReg(addr, H3LIS_REG_WHO_AM_I, &whoAmI, 1) || whoAmI != H3LIS_WHO_AM_I_VAL)
        return false;

    // CTRL_REG1: 1kHz ODR (DR=11), normal mode (PM=001), all axes enabled (XYZ=111)
    // Binary: 0011 1 111 = 0x3F
    if (!i2cWriteReg(addr, H3LIS_REG_CTRL1, 0x3F))
        return false;

    // CTRL_REG4: ±400g (FS=11), BDU=1 (block data update)
    // Binary: 1 11 0 0 000 = 0xC0 + 0x30 = 0xF0... actually:
    // Bit7=BDU, Bit6:5=00, Bit4=FS1, Bit3=FS0... FS=11 for ±400g
    // BDU=1, FS=11: 0x80 | 0x30 = 0xB0
    // Wait — H3LIS331: CTRL4 bit layout is BDU(7), BLE(6), FS1(5), FS0(4)
    // FS=11 for ±400g: bits 5:4 = 11 = 0x30. BDU=1: bit 7 = 0x80.
    if (!i2cWriteReg(addr, H3LIS_REG_CTRL4, 0xB0))
        return false;

    return true;
}

// --- Public API ---

bool h3lisInit() {
    initialized = false;

    // Enable I2C clock
#if defined(I2C_INSTANCE)
    // I2C instance from target.h/pinmap.h
    hi2c.Instance = I2C_INSTANCE;
#else
    hi2c.Instance = I2C1;  // Default
#endif

    // Configure I2C — different init structs for F4 (CCR-based) vs F7/H7 (TIMINGR-based)
#if defined(STM32F4xx)
    hi2c.Init.ClockSpeed = I2C_SPEED_HZ;
    hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
#else
    // F7/H7: TIMINGR value. 0x10707DBC = 400kHz at most common APB clocks.
    // Exact value depends on APB1 clock — verify with CubeMX at integration.
    hi2c.Init.Timing = 0x10707DBC;
#endif
    hi2c.Init.OwnAddress1 = 0;
    hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    // E-01: Bus recovery before init (handles both slave and master lockup)
    // The i2c_recovery module does:
    //   1. 9x SCL clock (clears latched slave)
    //   2. STOP condition
    //   3. RCC peripheral reset (clears F4 BUSY lockup erratum)
    //   4. HAL_I2C_Init()
    // For now, do a simple HAL init — recovery wiring at integration
    if (HAL_I2C_Init(&hi2c) != HAL_OK)
        return false;

    // Probe and configure both sensors
    bool innerOk = i2cProbe(innerAddr) && configureSensor(innerAddr);
    bool outerOk = i2cProbe(outerAddr) && configureSensor(outerAddr);

    // Heading needs BOTH sensors for differential measurement
    initialized = innerOk && outerOk;
    return initialized;
}

H3lisSensorPair h3lisReadPair() {
    H3lisSensorPair pair = {};

    if (!initialized) {
        return pair;
    }

    // Read centripetal axis only (2 bytes each, ~130µs per sensor at 400kHz I2C)
    // Default: Z-axis is centripetal (radial from spin axis)
    // Configurable at integration based on sensor mounting

    uint8_t rawInner[2] = {};
    uint8_t rawOuter[2] = {};

    pair.innerPresent = i2cReadReg(innerAddr, H3LIS_REG_OUT_Z_L, rawInner, 2);
    pair.outerPresent = i2cReadReg(outerAddr, H3LIS_REG_OUT_Z_L, rawOuter, 2);

    if (pair.innerPresent) {
        int16_t raw = (int16_t)((rawInner[1] << 8) | rawInner[0]) >> 4; // 12-bit left-justified
        pair.inner.z = (float)raw * H3LIS_SCALE_400G;
        pair.inner.valid = true;
    }

    if (pair.outerPresent) {
        int16_t raw = (int16_t)((rawOuter[1] << 8) | rawOuter[0]) >> 4;
        pair.outer.z = (float)raw * H3LIS_SCALE_400G;
        pair.outer.valid = true;
    }

    return pair;
}

H3lisReading h3lisReadXYZ(uint8_t addr) {
    H3lisReading reading = {};
    uint8_t raw[6] = {};

    if (!i2cReadReg(addr, H3LIS_REG_OUT_X_L, raw, 6))
        return reading;

    reading.x = (float)((int16_t)((raw[1] << 8) | raw[0]) >> 4) * H3LIS_SCALE_400G;
    reading.y = (float)((int16_t)((raw[3] << 8) | raw[2]) >> 4) * H3LIS_SCALE_400G;
    reading.z = (float)((int16_t)((raw[5] << 8) | raw[4]) >> 4) * H3LIS_SCALE_400G;
    reading.valid = true;
    return reading;
}

bool h3lisSensorsHealthy() {
    if (!initialized) return false;
    // Quick probe — just check if both respond
    return i2cProbe(innerAddr) && i2cProbe(outerAddr);
}

} // namespace melty

#endif // NATIVE_BUILD
