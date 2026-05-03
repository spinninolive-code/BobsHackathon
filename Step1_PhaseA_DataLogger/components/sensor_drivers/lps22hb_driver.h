/**
 * @file lps22hb_driver.h
 * @brief LPS22HB pressure and temperature sensor driver
 * 
 * Datasheet: https://www.st.com/resource/en/datasheet/lps22hb.pdf
 */

#ifndef LPS22HB_DRIVER_H
#define LPS22HB_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// LPS22HB handle
typedef void* lps22hb_handle_t;

/**
 * @brief Initialize LPS22HB sensor
 * 
 * @param module_id Module ID (0 or 1)
 * @param handle Pointer to store sensor handle
 * @return ESP_OK on success
 */
esp_err_t lps22hb_init(uint8_t module_id, lps22hb_handle_t *handle);

/**
 * @brief Read pressure and temperature
 * 
 * @param handle Sensor handle
 * @param pressure Pointer to store pressure (24-bit raw)
 * @param temperature Pointer to store temperature (16-bit raw)
 * @return ESP_OK on success
 */
esp_err_t lps22hb_read_press_temp(lps22hb_handle_t handle,
                                    int32_t *pressure, int16_t *temperature);

#ifdef __cplusplus
}
#endif

#endif // LPS22HB_DRIVER_H

// Made with Bob
