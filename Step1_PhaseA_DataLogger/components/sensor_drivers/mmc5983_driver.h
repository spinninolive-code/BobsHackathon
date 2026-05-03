/**
 * @file mmc5983_driver.h
 * @brief MMC5983MA 3-axis magnetometer driver
 * 
 * Datasheet: https://www.memsic.com/Public/Uploads/uploadfile/files/20220119/MMC5983MADatasheetRevA.pdf
 */

#ifndef MMC5983_DRIVER_H
#define MMC5983_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// MMC5983MA handle
typedef void* mmc5983_handle_t;

/**
 * @brief Initialize MMC5983MA sensor
 * 
 * @param module_id Module ID (0 or 1)
 * @param handle Pointer to store sensor handle
 * @return ESP_OK on success
 */
esp_err_t mmc5983_init(uint8_t module_id, mmc5983_handle_t *handle);

/**
 * @brief Read magnetometer data
 * 
 * @param handle Sensor handle
 * @param mag_x Pointer to store mag X (18-bit)
 * @param mag_y Pointer to store mag Y (18-bit)
 * @param mag_z Pointer to store mag Z (18-bit)
 * @return ESP_OK on success
 */
esp_err_t mmc5983_read_mag(mmc5983_handle_t handle,
                            int32_t *mag_x, int32_t *mag_y, int32_t *mag_z);

/**
 * @brief Perform SET/RESET operation for calibration
 * 
 * @param handle Sensor handle
 * @return ESP_OK on success
 */
esp_err_t mmc5983_set_reset(mmc5983_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // MMC5983_DRIVER_H

// Made with Bob
