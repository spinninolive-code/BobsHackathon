/**
 * @file accelerometer.h
 * @brief BMA250 Accelerometer Driver Interface
 * @version 1.0.0
 * @date 2026-05-03
 * 
 * Driver for Bosch BMA250 3-axis accelerometer via I2C
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "../config.h"
#include <Wire.h>

/**
 * @class Accelerometer
 * @brief BMA250 accelerometer driver class
 */
class Accelerometer {
public:
    /**
     * @brief Constructor
     */
    Accelerometer();
    
    /**
     * @brief Initialize the accelerometer
     * @return true if successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Read accelerometer data
     * @param x Pointer to store X-axis value
     * @param y Pointer to store Y-axis value
     * @param z Pointer to store Z-axis value
     * @return true if successful, false otherwise
     */
    bool read(int16_t* x, int16_t* y, int16_t* z);
    
    /**
     * @brief Read accelerometer data with error checking
     * @param x Pointer to store X-axis value
     * @param y Pointer to store Y-axis value
     * @param z Pointer to store Z-axis value
     * @return true if successful, false otherwise
     */
    bool readWithRetry(int16_t* x, int16_t* y, int16_t* z);
    
    /**
     * @brief Convert raw value to milligravity
     * @param raw Raw ADC value
     * @return Value in milligravity (mg)
     */
    int16_t rawToMilliG(int16_t raw);
    
    /**
     * @brief Convert raw value to g
     * @param raw Raw ADC value
     * @return Value in g
     */
    float rawToG(int16_t raw);
    
    /**
     * @brief Set accelerometer to suspend mode (low power)
     * @return true if successful, false otherwise
     */
    bool suspend();
    
    /**
     * @brief Wake accelerometer from suspend mode
     * @return true if successful, false otherwise
     */
    bool wakeup();
    
    /**
     * @brief Check if accelerometer is responding
     * @return true if responding, false otherwise
     */
    bool isConnected();
    
    /**
     * @brief Get chip ID
     * @return Chip ID value (should be 0x03)
     */
    uint8_t getChipID();
    
    /**
     * @brief Perform self-test
     * @return true if self-test passed, false otherwise
     */
    bool selfTest();
    
    /**
     * @brief Reset the accelerometer
     * @return true if successful, false otherwise
     */
    bool reset();
    
    /**
     * @brief Get last error code
     * @return Error code
     */
    ErrorCode getLastError() const { return lastError; }
    
    /**
     * @brief Get read count
     * @return Number of successful reads
     */
    uint32_t getReadCount() const { return readCount; }
    
    /**
     * @brief Get error count
     * @return Number of read errors
     */
    uint32_t getErrorCount() const { return errorCount; }

private:
    /**
     * @brief Write a byte to a register
     * @param reg Register address
     * @param value Value to write
     * @return true if successful, false otherwise
     */
    bool writeRegister(uint8_t reg, uint8_t value);
    
    /**
     * @brief Read a byte from a register
     * @param reg Register address
     * @param value Pointer to store read value
     * @return true if successful, false otherwise
     */
    bool readRegister(uint8_t reg, uint8_t* value);
    
    /**
     * @brief Read multiple bytes from registers
     * @param reg Starting register address
     * @param buffer Buffer to store read values
     * @param length Number of bytes to read
     * @return true if successful, false otherwise
     */
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
    
    /**
     * @brief Recover I2C bus after error
     * @return true if successful, false otherwise
     */
    bool recoverI2C();
    
    // Member variables
    bool initialized;
    ErrorCode lastError;
    uint32_t readCount;
    uint32_t errorCount;
    uint32_t lastReadTime;
};

#endif // ACCELEROMETER_H

// Made with Bob
