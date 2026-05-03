/**
 * @file sensor_drivers.h
 * @brief Common sensor driver interface for all sensor modules
 */

#ifndef SENSOR_DRIVERS_H
#define SENSOR_DRIVERS_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Single sensor sample (all sensors, one module)
 * 
 * This structure contains raw sensor data from all three sensors
 * on a single 10-DOF module.
 */
typedef struct {
    uint32_t timestamp_us;      ///< Microsecond timestamp
    
    // ICM-42688-P (Accel + Gyro)
    int16_t accel_x_raw;        ///< Raw accelerometer X
    int16_t accel_y_raw;        ///< Raw accelerometer Y
    int16_t accel_z_raw;        ///< Raw accelerometer Z
    int16_t gyro_x_raw;         ///< Raw gyroscope X
    int16_t gyro_y_raw;         ///< Raw gyroscope Y
    int16_t gyro_z_raw;         ///< Raw gyroscope Z
    
    // MMC5983MA (Magnetometer)
    int32_t mag_x_raw;          ///< Raw magnetometer X (18-bit)
    int32_t mag_y_raw;          ///< Raw magnetometer Y (18-bit)
    int32_t mag_z_raw;          ///< Raw magnetometer Z (18-bit)
    
    // LPS22HB (Pressure)
    int32_t pressure_raw;       ///< Raw pressure (24-bit)
    int16_t temperature_raw;    ///< Raw temperature (16-bit)
    
    uint8_t module_id;          ///< 0 or 1 (which sensor module)
    uint8_t status;             ///< Status flags
} __attribute__((packed)) sensor_sample_t;

// Size: 4 + 12 + 12 + 6 + 2 = 36 bytes per module

/**
 * @brief Converted/calibrated sensor data
 */
typedef struct {
    uint32_t timestamp_us;
    
    // Accelerometer (m/s²)
    float accel_x_ms2;
    float accel_y_ms2;
    float accel_z_ms2;
    
    // Gyroscope (rad/s)
    float gyro_x_rads;
    float gyro_y_rads;
    float gyro_z_rads;
    
    // Magnetometer (μT)
    float mag_x_ut;
    float mag_y_ut;
    float mag_z_ut;
    
    // Pressure (hPa) and Temperature (°C)
    float pressure_hpa;
    float temperature_c;
    
    uint8_t module_id;
    uint8_t quality;            ///< Data quality indicator
} sensor_data_t;

// ============================================================================
// Public API
// ============================================================================

/**
 * @brief Initialize all sensor drivers
 * 
 * @param spi_mutex Mutex for SPI bus access
 * @return ESP_OK on success
 */
esp_err_t sensor_drivers_init(SemaphoreHandle_t spi_mutex);

/**
 * @brief Read all sensors from a specific module
 * 
 * @param module_id Module ID (0 or 1)
 * @param sample Pointer to store sensor data
 * @return ESP_OK on success
 */
esp_err_t sensor_read_all_module(uint8_t module_id, sensor_sample_t *sample);

/**
 * @brief Convert raw sensor data to engineering units
 * 
 * @param raw Raw sensor sample
 * @param converted Pointer to store converted data
 * @return ESP_OK on success
 */
esp_err_t sensor_convert_data(const sensor_sample_t *raw, sensor_data_t *converted);

/**
 * @brief Calibrate sensors
 * 
 * @param module_id Module ID (0 or 1)
 * @return ESP_OK on success
 */
esp_err_t sensor_calibrate(uint8_t module_id);

/**
 * @brief Get sensor status
 * 
 * @param module_id Module ID (0 or 1)
 * @return true if sensor is healthy
 */
bool sensor_is_healthy(uint8_t module_id);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_DRIVERS_H

// Made with Bob
