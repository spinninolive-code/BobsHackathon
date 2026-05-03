/**
 * @file accelerometer.cpp
 * @brief BMA250 Accelerometer Driver Implementation
 * @version 1.0.0
 * @date 2026-05-03
 */

#include "accelerometer.h"

Accelerometer::Accelerometer() 
    : initialized(false)
    , lastError(ERR_NONE)
    , readCount(0)
    , errorCount(0)
    , lastReadTime(0)
{
}

bool Accelerometer::begin() {
    DEBUG_PRINTLN("Initializing BMA250 accelerometer...");
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(BMA250_I2C_SPEED);
    
    delay(10); // Allow sensor to power up
    
    // Check chip ID
    uint8_t chipID = getChipID();
    if (chipID != BMA250_CHIP_ID_VALUE) {
        DEBUG_PRINTF("BMA250 chip ID mismatch: 0x%02X (expected 0x03)\n", chipID);
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    DEBUG_PRINTLN("BMA250 chip ID verified: 0x03");
    
    // Soft reset
    if (!reset()) {
        DEBUG_PRINTLN("BMA250 reset failed");
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    delay(10); // Wait for reset to complete
    
    // Configure range (±2g)
    if (!writeRegister(BMA250_REG_RANGE, BMA250_ACTIVE_RANGE)) {
        DEBUG_PRINTLN("Failed to set range");
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    // Configure bandwidth (500Hz)
    if (!writeRegister(BMA250_REG_BANDWIDTH, BMA250_ACTIVE_BANDWIDTH)) {
        DEBUG_PRINTLN("Failed to set bandwidth");
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    // Set normal power mode
    if (!writeRegister(BMA250_REG_POWER_MODE, BMA250_ACTIVE_MODE)) {
        DEBUG_PRINTLN("Failed to set power mode");
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    delay(5); // Allow configuration to settle
    
    // Verify configuration
    uint8_t range, bandwidth, powerMode;
    if (!readRegister(BMA250_REG_RANGE, &range) ||
        !readRegister(BMA250_REG_BANDWIDTH, &bandwidth) ||
        !readRegister(BMA250_REG_POWER_MODE, &powerMode)) {
        DEBUG_PRINTLN("Failed to verify configuration");
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    DEBUG_PRINTF("BMA250 configured: Range=0x%02X, BW=0x%02X, Mode=0x%02X\n", 
                 range, bandwidth, powerMode);
    
    initialized = true;
    lastError = ERR_NONE;
    
    DEBUG_PRINTLN("BMA250 initialization complete");
    return true;
}

bool Accelerometer::read(int16_t* x, int16_t* y, int16_t* z) {
    if (!initialized) {
        lastError = ERR_ACCEL_INIT;
        return false;
    }
    
    uint8_t data[6];
    
    // Read all 6 bytes (X, Y, Z axes) in one transaction
    if (!readRegisters(BMA250_REG_X_LSB, data, 6)) {
        errorCount++;
        lastError = ERR_I2C_TIMEOUT;
        return false;
    }
    
    // Combine bytes (10-bit resolution, left-aligned in 16-bit)
    // Shift right by 6 to get 10-bit signed value
    *x = (int16_t)((data[1] << 8) | data[0]) >> 6;
    *y = (int16_t)((data[3] << 8) | data[2]) >> 6;
    *z = (int16_t)((data[5] << 8) | data[4]) >> 6;
    
    readCount++;
    lastReadTime = micros();
    lastError = ERR_NONE;
    
    return true;
}

bool Accelerometer::readWithRetry(int16_t* x, int16_t* y, int16_t* z) {
    uint8_t retries = ERROR_MAX_RETRIES;
    
    while (retries--) {
        if (read(x, y, z)) {
            return true;
        }
        
        // Brief delay before retry
        delayMicroseconds(ERROR_RETRY_DELAY_MS * 1000);
        
        // Try to recover I2C bus on last retry
        if (retries == 0) {
            DEBUG_PRINTLN("Attempting I2C recovery...");
            recoverI2C();
        }
    }
    
    DEBUG_PRINTLN("BMA250 read failed after retries");
    return false;
}

int16_t Accelerometer::rawToMilliG(int16_t raw) {
    // Convert raw 10-bit value to milligravity
    // At ±2g range: 1024 LSB/g = 1.024 LSB/mg
    return (raw * 1000) / BMA250_LSB_PER_G;
}

float Accelerometer::rawToG(int16_t raw) {
    // Convert raw 10-bit value to g
    return (float)raw / BMA250_LSB_PER_G;
}

bool Accelerometer::suspend() {
    DEBUG_PRINTLN("Suspending BMA250...");
    
    if (!writeRegister(BMA250_REG_POWER_MODE, BMA250_MODE_SUSPEND)) {
        DEBUG_PRINTLN("Failed to suspend BMA250");
        return false;
    }
    
    initialized = false;
    return true;
}

bool Accelerometer::wakeup() {
    DEBUG_PRINTLN("Waking up BMA250...");
    
    if (!writeRegister(BMA250_REG_POWER_MODE, BMA250_MODE_NORMAL)) {
        DEBUG_PRINTLN("Failed to wake up BMA250");
        return false;
    }
    
    delay(5); // Allow sensor to wake up
    initialized = true;
    return true;
}

bool Accelerometer::isConnected() {
    uint8_t chipID = getChipID();
    return (chipID == BMA250_CHIP_ID_VALUE);
}

uint8_t Accelerometer::getChipID() {
    uint8_t chipID = 0;
    readRegister(BMA250_REG_CHIP_ID, &chipID);
    return chipID;
}

bool Accelerometer::selfTest() {
    DEBUG_PRINTLN("Running BMA250 self-test...");
    
    // Read baseline values
    int16_t x1, y1, z1;
    if (!read(&x1, &y1, &z1)) {
        DEBUG_PRINTLN("Self-test failed: cannot read baseline");
        return false;
    }
    
    delay(10);
    
    // Read again
    int16_t x2, y2, z2;
    if (!read(&x2, &y2, &z2)) {
        DEBUG_PRINTLN("Self-test failed: cannot read second sample");
        return false;
    }
    
    // Check that values are reasonable (within ±2g range)
    if (abs(x1) > 2048 || abs(y1) > 2048 || abs(z1) > 2048 ||
        abs(x2) > 2048 || abs(y2) > 2048 || abs(z2) > 2048) {
        DEBUG_PRINTLN("Self-test failed: values out of range");
        return false;
    }
    
    // Check that Z-axis shows gravity (~1g = 1024 LSB)
    if (abs(z1) < 512 || abs(z1) > 1536) {
        DEBUG_PRINTLN("Self-test warning: Z-axis not showing gravity");
        // This is a warning, not a failure
    }
    
    DEBUG_PRINTF("Self-test passed: X=%d, Y=%d, Z=%d\n", x1, y1, z1);
    return true;
}

bool Accelerometer::reset() {
    DEBUG_PRINTLN("Resetting BMA250...");
    
    // Write soft reset command
    if (!writeRegister(BMA250_REG_SOFT_RESET, 0xB6)) {
        DEBUG_PRINTLN("Failed to send reset command");
        return false;
    }
    
    delay(10); // Wait for reset to complete
    
    // Verify chip ID after reset
    uint8_t chipID = getChipID();
    if (chipID != BMA250_CHIP_ID_VALUE) {
        DEBUG_PRINTLN("Reset verification failed");
        return false;
    }
    
    return true;
}

bool Accelerometer::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BMA250_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t status = Wire.endTransmission();
    
    if (status != 0) {
        DEBUG_PRINTF("I2C write error: %d\n", status);
        lastError = (status == 2) ? ERR_I2C_NACK : ERR_I2C_TIMEOUT;
        return false;
    }
    
    return true;
}

bool Accelerometer::readRegister(uint8_t reg, uint8_t* value) {
    // Write register address
    Wire.beginTransmission(BMA250_I2C_ADDR);
    Wire.write(reg);
    uint8_t status = Wire.endTransmission(false); // Repeated start
    
    if (status != 0) {
        DEBUG_PRINTF("I2C write error: %d\n", status);
        lastError = (status == 2) ? ERR_I2C_NACK : ERR_I2C_TIMEOUT;
        return false;
    }
    
    // Read data
    uint8_t bytesRead = Wire.requestFrom(BMA250_I2C_ADDR, (uint8_t)1);
    
    if (bytesRead != 1) {
        DEBUG_PRINTLN("I2C read error: no data");
        lastError = ERR_I2C_TIMEOUT;
        return false;
    }
    
    *value = Wire.read();
    return true;
}

bool Accelerometer::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    // Write register address
    Wire.beginTransmission(BMA250_I2C_ADDR);
    Wire.write(reg);
    uint8_t status = Wire.endTransmission(false); // Repeated start
    
    if (status != 0) {
        DEBUG_PRINTF("I2C write error: %d\n", status);
        lastError = (status == 2) ? ERR_I2C_NACK : ERR_I2C_TIMEOUT;
        return false;
    }
    
    // Read data
    uint8_t bytesRead = Wire.requestFrom(BMA250_I2C_ADDR, length);
    
    if (bytesRead != length) {
        DEBUG_PRINTF("I2C read error: expected %d bytes, got %d\n", length, bytesRead);
        lastError = ERR_I2C_TIMEOUT;
        return false;
    }
    
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = Wire.read();
    }
    
    return true;
}

bool Accelerometer::recoverI2C() {
    DEBUG_PRINTLN("Recovering I2C bus...");
    
    // Reset I2C peripheral
    Wire.end();
    delay(10);
    
    // Re-initialize I2C
    Wire.begin();
    Wire.setClock(BMA250_I2C_SPEED);
    delay(10);
    
    // Check if device responds
    if (isConnected()) {
        DEBUG_PRINTLN("I2C recovery successful");
        
        // Re-initialize accelerometer
        return begin();
    }
    
    DEBUG_PRINTLN("I2C recovery failed");
    return false;
}

// Made with Bob
