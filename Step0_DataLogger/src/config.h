/**
 * @file config.h
 * @brief System configuration and pin definitions for Teensy 3.1 Data Logger
 * @version 1.0.0
 * @date 2026-05-03
 * 
 * Hardware: Teensy 3.1 + BMA250 Accelerometer + SD Card Module
 * Sample Rate: 1kHz
 * Storage: CSV format on SD card
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_DATE "2026-05-03"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Control Switch Pins (INPUT_PULLUP)
#define PIN_SWITCH_OFF      2   // Switch position 1 (OFF)
#define PIN_SWITCH_IDLE     3   // Switch position 2 (IDLE)
#define PIN_SWITCH_LOG      4   // Switch position 3 (LOGGING)

// Status LED Pins (OUTPUT)
#define PIN_LED_STATUS      5   // Green LED - System status
#define PIN_LED_ERROR       6   // Red LED - Error indication
#define PIN_LED_LOGGING     7   // Blue LED - Logging active

// Battery Monitor (ADC)
#define PIN_BATTERY_MON     A0  // Battery voltage divider

// I2C Pins (BMA250 Accelerometer)
#define PIN_I2C_SDA         18  // I2C0 SDA (A4)
#define PIN_I2C_SCL         19  // I2C0 SCL (A5)

// SPI Pins (SD Card Module)
#define PIN_SD_CS           10  // SD Card Chip Select
#define PIN_SPI_MOSI        11  // SPI0 MOSI
#define PIN_SPI_MISO        12  // SPI0 MISO
#define PIN_SPI_SCK         13  // SPI0 SCK

// ============================================================================
// BMA250 ACCELEROMETER CONFIGURATION
// ============================================================================

// I2C Configuration
#define BMA250_I2C_ADDR         0x18    // BMA250 I2C address
#define BMA250_I2C_SPEED        400000  // 400kHz Fast Mode
#define BMA250_I2C_TIMEOUT      10      // Timeout in milliseconds

// BMA250 Register Addresses
#define BMA250_REG_CHIP_ID      0x00    // Should read 0x03
#define BMA250_REG_X_LSB        0x02    // X-axis LSB
#define BMA250_REG_X_MSB        0x03    // X-axis MSB
#define BMA250_REG_Y_LSB        0x04    // Y-axis LSB
#define BMA250_REG_Y_MSB        0x05    // Y-axis MSB
#define BMA250_REG_Z_LSB        0x06    // Z-axis LSB
#define BMA250_REG_Z_MSB        0x07    // Z-axis MSB
#define BMA250_REG_TEMP         0x08    // Temperature
#define BMA250_REG_STATUS       0x09    // Status register
#define BMA250_REG_RANGE        0x0F    // Range selection
#define BMA250_REG_BANDWIDTH    0x10    // Bandwidth selection
#define BMA250_REG_POWER_MODE   0x11    // Power mode control
#define BMA250_REG_SOFT_RESET   0x14    // Soft reset

// BMA250 Configuration Values
#define BMA250_CHIP_ID_VALUE    0x03    // Expected chip ID
#define BMA250_RANGE_2G         0x03    // ±2g range
#define BMA250_RANGE_4G         0x05    // ±4g range
#define BMA250_RANGE_8G         0x08    // ±8g range
#define BMA250_RANGE_16G        0x0C    // ±16g range

#define BMA250_BW_7_81HZ        0x08    // 7.81 Hz bandwidth
#define BMA250_BW_15_63HZ       0x09    // 15.63 Hz bandwidth
#define BMA250_BW_31_25HZ       0x0A    // 31.25 Hz bandwidth
#define BMA250_BW_62_5HZ        0x0B    // 62.5 Hz bandwidth
#define BMA250_BW_125HZ         0x0C    // 125 Hz bandwidth
#define BMA250_BW_250HZ         0x0D    // 250 Hz bandwidth
#define BMA250_BW_500HZ         0x0E    // 500 Hz bandwidth
#define BMA250_BW_1000HZ        0x0F    // 1000 Hz bandwidth

#define BMA250_MODE_NORMAL      0x00    // Normal mode
#define BMA250_MODE_SUSPEND     0x80    // Suspend mode

// Active Configuration
#define BMA250_ACTIVE_RANGE     BMA250_RANGE_2G     // ±2g
#define BMA250_ACTIVE_BANDWIDTH BMA250_BW_500HZ     // 500Hz (Nyquist for 1kHz)
#define BMA250_ACTIVE_MODE      BMA250_MODE_NORMAL  // Normal mode

// Conversion Constants
#define BMA250_LSB_PER_G        1024    // 10-bit resolution at ±2g
#define BMA250_MG_PER_LSB       (1000.0 / BMA250_LSB_PER_G)

// ============================================================================
// DATA ACQUISITION CONFIGURATION
// ============================================================================

// Sampling Configuration
#define SAMPLE_RATE_HZ          1000    // 1kHz sampling rate
#define SAMPLE_INTERVAL_US      1000    // 1ms interval (1000 microseconds)
#define SAMPLE_TOLERANCE_PCT    1       // ±1% tolerance

// Data Buffer Configuration
#define BUFFER_SIZE             5461    // ~5.5 seconds at 1kHz
#define WRITE_THRESHOLD         512     // Write every 512 samples (0.512s)
#define BUFFER_WARNING_PCT      90      // Warn at 90% full

// Timing Budget (microseconds)
#define TIMING_BUDGET_US        1000    // Total 1ms period
#define TIMING_I2C_MAX_US       200     // I2C read max time
#define TIMING_BUFFER_MAX_US    50      // Buffer write max time
#define TIMING_OVERHEAD_US      50      // ISR overhead

// ============================================================================
// SD CARD CONFIGURATION
// ============================================================================

// SPI Configuration
#define SD_SPI_SPEED            25000000    // 25MHz
#define SD_CS_PIN               PIN_SD_CS

// File System Configuration
#define SD_FILENAME_PREFIX      "LOG_"
#define SD_FILENAME_EXT         ".csv"
#define SD_MAX_FILENAME_LEN     32

// Storage Thresholds
#define SD_MIN_FREE_SPACE_MB    100     // Minimum 100MB free
#define SD_WARN_FREE_SPACE_PCT  10      // Warn at 10% remaining
#define SD_STOP_FREE_SPACE_PCT  5       // Stop at 5% remaining

// Write Configuration
#define SD_WRITE_BUFFER_SIZE    512     // Samples per write
#define SD_FLUSH_INTERVAL_MS    500     // Flush every 0.5 seconds
#define SD_WRITE_RETRIES        3       // Retry failed writes 3 times

// ============================================================================
// POWER MANAGEMENT CONFIGURATION
// ============================================================================

// Battery Voltage Thresholds (in volts)
#define BATTERY_FULL_V          4.2     // 100% charged
#define BATTERY_GOOD_V          3.7     // Normal operation
#define BATTERY_LOW_V           3.5     // Low battery warning
#define BATTERY_CRITICAL_V      3.3     // Critical - stop logging
#define BATTERY_CUTOFF_V        3.0     // Emergency shutdown

// Battery Monitoring
#define BATTERY_CHECK_INTERVAL_MS   1000    // Check every 1 second
#define BATTERY_VOLTAGE_DIVIDER     2.0     // Voltage divider ratio
#define BATTERY_ADC_SAMPLES         10      // Average 10 samples

// Power States Current Draw (mA)
#define POWER_OFF_MA            1       // OFF state
#define POWER_IDLE_MA           7       // IDLE state
#define POWER_LOGGING_MA        135     // LOGGING state

// CPU Clock Speeds
#define CPU_CLOCK_LOGGING_MHZ   48      // Full speed for logging
#define CPU_CLOCK_IDLE_MHZ      24      // Reduced speed for idle

// ============================================================================
// SYSTEM STATE CONFIGURATION
// ============================================================================

// System States
enum SystemState {
    STATE_OFF = 0,          // System powered down
    STATE_IDLE,             // Ready, waiting for command
    STATE_LOGGING,          // Active data acquisition
    STATE_ERROR,            // Error condition
    STATE_LOW_BATTERY,      // Low battery, safe shutdown
    STATE_SHUTDOWN          // Shutting down
};

// Switch Debounce
#define SWITCH_DEBOUNCE_MS      50      // 50ms debounce time
#define SWITCH_SAMPLE_INTERVAL  5       // Sample every 5ms

// ============================================================================
// LED CONFIGURATION
// ============================================================================

// LED Patterns (blink rates in Hz)
#define LED_PATTERN_OFF         0       // LED off
#define LED_PATTERN_ON          -1      // LED solid on
#define LED_PATTERN_SLOW        1       // 1Hz slow blink
#define LED_PATTERN_MEDIUM      2       // 2Hz medium blink
#define LED_PATTERN_FAST        5       // 5Hz fast blink
#define LED_PATTERN_VFAST       10      // 10Hz very fast blink

// LED Brightness (0-255)
#define LED_BRIGHTNESS_FULL     255     // Full brightness
#define LED_BRIGHTNESS_DIM      64      // Dim brightness
#define LED_BRIGHTNESS_OFF      0       // Off

// ============================================================================
// ERROR HANDLING CONFIGURATION
// ============================================================================

// Error Codes
enum ErrorCode {
    ERR_NONE = 0x00,                // No error
    ERR_I2C_TIMEOUT = 0x01,         // I2C transaction timeout
    ERR_I2C_NACK = 0x02,            // I2C device not responding
    ERR_ACCEL_INIT = 0x03,          // Accelerometer init failed
    ERR_SD_INIT = 0x04,             // SD card init failed
    ERR_SD_WRITE = 0x05,            // SD write failed
    ERR_SD_FULL = 0x06,             // SD card full
    ERR_BUFFER_OVERFLOW = 0x07,     // Data buffer overflow
    ERR_BATTERY_LOW = 0x08,         // Battery voltage low
    ERR_BATTERY_CRITICAL = 0x09,    // Battery critically low
    ERR_FILE_CREATE = 0x0A,         // Cannot create file
    ERR_FILE_CLOSE = 0x0B,          // Cannot close file
    ERR_TIMESTAMP_OVERFLOW = 0x0C   // Timestamp overflow
};

// Error Severity Levels
enum ErrorSeverity {
    SEVERITY_INFO = 0,      // Informational
    SEVERITY_WARNING,       // Non-critical issue
    SEVERITY_ERROR,         // Recoverable error
    SEVERITY_CRITICAL       // System failure
};

// Error Retry Configuration
#define ERROR_MAX_RETRIES       3       // Maximum retry attempts
#define ERROR_RETRY_DELAY_MS    10      // Delay between retries
#define ERROR_HISTORY_SIZE      16      // Number of errors to store

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Accelerometer Sample Structure
struct AccelSample {
    uint32_t timestamp_ms;  // Milliseconds since start
    int16_t x;              // X-axis (raw ADC value)
    int16_t y;              // Y-axis (raw ADC value)
    int16_t z;              // Z-axis (raw ADC value)
};

// System Status Structure
struct SystemStatus {
    SystemState state;
    uint32_t uptime_ms;
    uint32_t samples_acquired;
    uint32_t samples_written;
    uint32_t errors_count;
    float battery_voltage;
    uint8_t battery_percent;
    bool sd_card_present;
    uint32_t sd_free_space_mb;
};

// ============================================================================
// MEMORY CONFIGURATION
// ============================================================================

// Stack and Heap
#define STACK_SIZE_BYTES        8192    // 8KB stack
#define HEAP_SIZE_BYTES         10240   // 10KB heap

// Buffer Memory
#define BUFFER_MEMORY_BYTES     (BUFFER_SIZE * sizeof(AccelSample))
#define SD_BUFFER_BYTES         4096    // 4KB SD write buffer
#define I2C_BUFFER_BYTES        256     // 256B I2C buffer
#define ERROR_LOG_BYTES         1024    // 1KB error log

// Total RAM Usage Estimate
#define TOTAL_RAM_ESTIMATE      (BUFFER_MEMORY_BYTES + SD_BUFFER_BYTES + \
                                 I2C_BUFFER_BYTES + ERROR_LOG_BYTES + \
                                 STACK_SIZE_BYTES + HEAP_SIZE_BYTES)

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

// Debug Output
#define DEBUG_ENABLED           1       // Enable debug output
#define DEBUG_SERIAL_BAUD       115200  // Serial baud rate
#define DEBUG_BUFFER_SIZE       256     // Debug message buffer

// Debug Levels
#define DEBUG_LEVEL_NONE        0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_WARNING     2
#define DEBUG_LEVEL_INFO        3
#define DEBUG_LEVEL_VERBOSE     4

#define DEBUG_LEVEL             DEBUG_LEVEL_INFO

// Debug Macros
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// ============================================================================
// TIMING MACROS
// ============================================================================

#define MILLIS()                millis()
#define MICROS()                micros()
#define DELAY_MS(x)             delay(x)
#define DELAY_US(x)             delayMicroseconds(x)

// ============================================================================
// UTILITY MACROS
// ============================================================================

#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define CONSTRAIN(x, a, b)      ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define MAP(x, in_min, in_max, out_min, out_max) \
    (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

// ============================================================================
// COMPILE-TIME CHECKS
// ============================================================================

// Verify buffer size is reasonable
#if BUFFER_SIZE < 100
    #error "BUFFER_SIZE too small (minimum 100 samples)"
#endif

#if BUFFER_SIZE > 10000
    #error "BUFFER_SIZE too large (maximum 10000 samples)"
#endif

// Verify sample rate is achievable
#if SAMPLE_RATE_HZ < 1 || SAMPLE_RATE_HZ > 2000
    #error "SAMPLE_RATE_HZ out of range (1-2000 Hz)"
#endif

// Verify timing budget
#if (TIMING_I2C_MAX_US + TIMING_BUFFER_MAX_US + TIMING_OVERHEAD_US) > TIMING_BUDGET_US
    #error "Timing budget exceeded - reduce operation times"
#endif

// Verify RAM usage
#if TOTAL_RAM_ESTIMATE > 60000
    #warning "RAM usage may exceed available memory (64KB)"
#endif

#endif // CONFIG_H

// Made with Bob
