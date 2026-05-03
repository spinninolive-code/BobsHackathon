/**
 * @file icm42688_driver.h
 * @brief ICM-42688-P 6-axis IMU driver (accelerometer + gyroscope)
 * 
 * Datasheet: https://invensense.tdk.com/download-resource/ds-000347-icm-42688-p-datasheet
 */

#ifndef ICM42688_DRIVER_H
#define ICM42688_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ICM-42688-P handle
typedef void* icm42688_handle_t;

// Register addresses
#define ICM42688_WHO_AM_I           0x75
#define ICM42688_WHO_AM_I_VALUE     0x47

#define ICM42688_PWR_MGMT0          0x4E
#define ICM42688_GYRO_CONFIG0       0x4F
#define ICM42688_ACCEL_CONFIG0      0x50

#define ICM42688_TEMP_DATA1         0x1D
#define ICM42688_ACCEL_DATA_X1      0x1F
#define ICM42688_GYRO_DATA_X1       0x25

// Configuration
#define ICM42688_ACCEL_RANGE_16G    0x00
#define ICM42688_GYRO_RANGE_2000DPS 0x00
#define ICM42688_ODR_1KHZ           0x06

/**
 * @brief Initialize ICM-42688-P sensor
 * 
 * @param module_id Module ID (0 or 1)
 * @param handle Pointer to store sensor handle
 * @return ESP_OK on success
 */
esp_err_t icm42688_init(uint8_t module_id, icm42688_handle_t *handle);

/**
 * @brief Read accelerometer and gyroscope data
 * 
 * @param handle Sensor handle
 * @param accel_x Pointer to store accel X
 * @param accel_y Pointer to store accel Y
 * @param accel_z Pointer to store accel Z
 * @param gyro_x Pointer to store gyro X
 * @param gyro_y Pointer to store gyro Y
 * @param gyro_z Pointer to store gyro Z
 * @return ESP_OK on success
 */
esp_err_t icm42688_read_accel_gyro(icm42688_handle_t handle,
                                     int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                     int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);

/**
 * @brief Calibrate sensor (zero offset)
 * 
 * @param handle Sensor handle
 * @return ESP_OK on success
 */
esp_err_t icm42688_calibrate(icm42688_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // ICM42688_DRIVER_H

// Made with Bob
