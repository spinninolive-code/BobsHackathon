/**
 * @file power_manager.h
 * @brief Battery and power management interface
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Battery status structure
 */
typedef struct {
    float voltage_mv;           ///< Battery voltage in millivolts
    uint8_t percentage;         ///< Battery percentage (0-100)
    bool charging;              ///< Charging status
    bool low_battery;           ///< Low battery warning flag
    bool critical_battery;      ///< Critical battery flag
} battery_status_t;

/**
 * @brief Power state enumeration
 */
typedef enum {
    POWER_STATE_NORMAL,         ///< Normal operation
    POWER_STATE_LOW_BATTERY,    ///< Low battery warning
    POWER_STATE_CRITICAL,       ///< Critical battery level
    POWER_STATE_CHARGING        ///< Battery charging
} power_state_t;

/**
 * @brief Battery event callback function type
 * 
 * @param state New power state
 */
typedef void (*battery_event_callback_t)(power_state_t state);

/**
 * @brief Initialize power manager
 * 
 * Configures ADC for battery voltage monitoring
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t power_manager_init(void);

/**
 * @brief Get battery status
 * 
 * Reads ADC and calculates battery voltage and percentage
 * 
 * @return Battery status structure
 */
battery_status_t power_manager_get_status(void);

/**
 * @brief Get battery voltage in millivolts
 * 
 * @return Battery voltage in mV
 */
float power_manager_get_voltage(void);

/**
 * @brief Get battery percentage
 * 
 * @return Battery percentage (0-100)
 */
uint8_t power_manager_get_percentage(void);

/**
 * @brief Check if battery is charging
 * 
 * @return true if charging
 */
bool power_manager_is_charging(void);

/**
 * @brief Check if battery is low
 * 
 * @return true if battery is below low threshold
 */
bool power_manager_is_low_battery(void);

/**
 * @brief Check if battery is critical
 * 
 * @return true if battery is below critical threshold
 */
bool power_manager_is_critical_battery(void);

/**
 * @brief Register battery event callback
 * 
 * @param callback Callback function to register
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t power_manager_register_callback(battery_event_callback_t callback);

/**
 * @brief Get current power state
 * 
 * @return Current power state
 */
power_state_t power_manager_get_state(void);

/**
 * @brief Get power state name string
 * 
 * @param state Power state
 * @return State name string
 */
const char* power_manager_get_state_name(power_state_t state);

#ifdef __cplusplus
}
#endif

#endif // POWER_MANAGER_H

// Made with Bob
