/**
 * @file app_config.h
 * @brief Application configuration for Phase A Data Logger
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

// ============================================================================
// GPIO Pin Definitions
// ============================================================================

// Sensor SPI Bus
#define PIN_SPI_MOSI        35
#define PIN_SPI_MISO        36
#define PIN_SPI_CLK         37

// Sensor Chip Select Pins
#define PIN_SENSOR1_CS      1   // ICM-42688-P Module 1
#define PIN_SENSOR2_CS      2   // ICM-42688-P Module 2
#define PIN_MAG1_CS         3   // MMC5983MA Module 1
#define PIN_MAG2_CS         4   // MMC5983MA Module 2
#define PIN_PRESS1_CS       5   // LPS22HB Module 1
#define PIN_PRESS2_CS       6   // LPS22HB Module 2

// SD Card SDIO Interface
#define PIN_SD_D0           12
#define PIN_SD_D1           13
#define PIN_SD_D2           14
#define PIN_SD_D3           15
#define PIN_SD_CLK          16
#define PIN_SD_CMD          17

// User Interface
#define PIN_DIP_SW1         7   // Mode select bit 0
#define PIN_DIP_SW2         8   // Mode select bit 1
#define PIN_DIP_SW3         9   // Mode select bit 2
#define PIN_STATUS_LED      11  // WS2812 RGB LED

// Power Management
#define PIN_BATTERY_ADC     10  // ADC1_CHANNEL_0

// ============================================================================
// SPI Configuration
// ============================================================================

#define SPI_HOST            SPI2_HOST
#define SPI_CLOCK_SPEED     (20 * 1000 * 1000)  // 20 MHz
#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO

// ============================================================================
// Sensor Configuration
// ============================================================================

// ICM-42688-P Configuration
#define ICM_ACCEL_RANGE     3   // 0=±2g, 1=±4g, 2=±8g, 3=±16g
#define ICM_GYRO_RANGE      3   // 0=±250dps, 1=±500dps, 2=±1000dps, 3=±2000dps
#define ICM_ACCEL_ODR       1000  // Hz
#define ICM_GYRO_ODR        1000  // Hz

// MMC5983MA Configuration
#define MMC_ODR             1000  // Hz
#define MMC_CONTINUOUS_MODE true

// LPS22HB Configuration
#define LPS_ODR             75    // Hz (max for this sensor)

// ============================================================================
// Data Acquisition Configuration
// ============================================================================

#define SAMPLE_RATE_HZ      1000
#define SAMPLE_PERIOD_US    (1000000 / SAMPLE_RATE_HZ)

// Number of sensor modules
#define NUM_SENSOR_MODULES  2

// ============================================================================
// Buffer Configuration
// ============================================================================

// PSRAM Ring Buffer
#define PSRAM_BUFFER_SIZE       (4 * 1024 * 1024)  // 4MB
#define PSRAM_FLUSH_THRESHOLD   (PSRAM_BUFFER_SIZE / 2)  // Flush at 50%

// NAND Flash Cache
#define FLASH_CACHE_SIZE        (8 * 1024 * 1024)  // 8MB
#define FLASH_FLUSH_THRESHOLD   (FLASH_CACHE_SIZE * 3 / 4)  // Flush at 75%
#define FLASH_BLOCK_SIZE        (4 * 1024)  // 4KB blocks

// SD Card Write Buffer
#define SD_WRITE_BUFFER_SIZE    (512 * 1024)  // 512KB
#define SD_WRITE_THRESHOLD      (256 * 1024)  // Write at 256KB

// ============================================================================
// Power Management Configuration
// ============================================================================

// Battery voltage thresholds (mV)
#define BATTERY_FULL            4200
#define BATTERY_NOMINAL         3700
#define BATTERY_LOW             3400
#define BATTERY_CRITICAL        3200
#define BATTERY_SHUTDOWN        3000

// Battery voltage divider (R1=R2=10kΩ)
#define BATTERY_VOLTAGE_DIVIDER 2.0f

// ADC configuration
#define ADC_VREF                3.3f
#define ADC_RESOLUTION          4096.0f

// ============================================================================
// FreeRTOS Task Configuration
// ============================================================================

// Task priorities (0-24, higher = more priority)
#define TASK_PRIORITY_SENSOR    24  // Highest - time critical
#define TASK_PRIORITY_DATA      20  // High - data processing
#define TASK_PRIORITY_SD        15  // Medium-high - storage
#define TASK_PRIORITY_UI        8   // Medium-low - user interface
#define TASK_PRIORITY_MONITOR   5   // Low - battery/status

// Task stack sizes (bytes)
#define STACK_SIZE_SENSOR       4096
#define STACK_SIZE_DATA         8192
#define STACK_SIZE_SD           4096
#define STACK_SIZE_UI           2048
#define STACK_SIZE_MONITOR      2048

// ============================================================================
// File System Configuration
// ============================================================================

#define SD_MOUNT_POINT          "/sdcard"
#define SD_MAX_FILES            5
#define CSV_LINE_BUFFER_SIZE    256

// ============================================================================
// LED Status Colors (WS2812 RGB)
// ============================================================================

#define LED_OFF                 0x000000
#define LED_RED                 0xFF0000
#define LED_GREEN               0x00FF00
#define LED_BLUE                0x0000FF
#define LED_YELLOW              0xFFFF00
#define LED_CYAN                0x00FFFF
#define LED_MAGENTA             0xFF00FF
#define LED_WHITE               0xFFFFFF

// Status color mapping
#define LED_COLOR_OFF           LED_OFF
#define LED_COLOR_IDLE          LED_BLUE
#define LED_COLOR_LOGGING       LED_GREEN
#define LED_COLOR_CALIBRATING   LED_YELLOW
#define LED_COLOR_TEST          LED_CYAN
#define LED_COLOR_ERROR         LED_RED
#define LED_COLOR_LOW_BATTERY   LED_MAGENTA

// ============================================================================
// Logging Configuration
// ============================================================================

#define LOG_TAG_MAIN            "MAIN"
#define LOG_TAG_SENSOR          "SENSOR"
#define LOG_TAG_BUFFER          "BUFFER"
#define LOG_TAG_SD              "SD"
#define LOG_TAG_STATE           "STATE"
#define LOG_TAG_POWER           "POWER"
#define LOG_TAG_UI              "UI"

// ============================================================================
// Version Information
// ============================================================================

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_STRING "1.0.0"

#endif // APP_CONFIG_H

// Made with Bob
