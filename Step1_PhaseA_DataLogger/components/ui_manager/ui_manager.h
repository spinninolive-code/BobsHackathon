/**
 * @file ui_manager.h
 * @brief User interface management (DIP switches and LED)
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DIP switch mode enumeration
 */
typedef enum {
    MODE_IDLE = 0,          ///< 000 - Idle mode
    MODE_ACQUIRE = 1,       ///< 001 - Acquire data
    MODE_STORE = 2,         ///< 010 - Store to SD
    MODE_ACQUIRE_STORE = 3, ///< 011 - Acquire and store
    MODE_TEST = 4,          ///< 100 - Test mode
    MODE_RESERVED_5 = 5,    ///< 101 - Reserved
    MODE_RESERVED_6 = 6,    ///< 110 - Reserved
    MODE_SHUTDOWN = 7       ///< 111 - Shutdown
} dip_mode_t;

/**
 * @brief LED color enumeration
 */
typedef enum {
    LED_OFF,                ///< LED off
    LED_RED,                ///< Red (error/critical)
    LED_GREEN,              ///< Green (normal/ready)
    LED_BLUE,               ///< Blue (acquiring)
    LED_YELLOW,             ///< Yellow (warning)
    LED_CYAN,               ///< Cyan (storing)
    LED_MAGENTA,            ///< Magenta (test)
    LED_WHITE               ///< White (idle)
} led_color_t;

/**
 * @brief LED pattern enumeration
 */
typedef enum {
    LED_PATTERN_SOLID,      ///< Solid color
    LED_PATTERN_BLINK_SLOW, ///< Slow blink (1Hz)
    LED_PATTERN_BLINK_FAST, ///< Fast blink (4Hz)
    LED_PATTERN_PULSE       ///< Breathing effect
} led_pattern_t;

/**
 * @brief Mode change callback function type
 * 
 * @param old_mode Previous mode
 * @param new_mode New mode
 */
typedef void (*mode_change_callback_t)(dip_mode_t old_mode, dip_mode_t new_mode);

/**
 * @brief Initialize UI manager
 * 
 * Configures DIP switch GPIOs and WS2812 LED
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_init(void);

/**
 * @brief Read DIP switch mode
 * 
 * @return Current DIP switch mode
 */
dip_mode_t ui_manager_read_mode(void);

/**
 * @brief Set LED color
 * 
 * @param color LED color to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_set_led_color(led_color_t color);

/**
 * @brief Set LED pattern
 * 
 * @param color LED color
 * @param pattern LED pattern
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_set_led_pattern(led_color_t color, led_pattern_t pattern);

/**
 * @brief Set LED RGB values directly
 * 
 * @param red Red value (0-255)
 * @param green Green value (0-255)
 * @param blue Blue value (0-255)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_set_led_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn LED off
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_led_off(void);

/**
 * @brief Register mode change callback
 * 
 * @param callback Callback function to register
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_register_callback(mode_change_callback_t callback);

/**
 * @brief Get mode name string
 * 
 * @param mode Mode to get name for
 * @return Mode name string
 */
const char* ui_manager_get_mode_name(dip_mode_t mode);

/**
 * @brief Update LED based on system state
 * 
 * Helper function to set LED color/pattern based on common states
 * 
 * @param state_name State name (e.g., "IDLE", "ACQUIRING", "ERROR")
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ui_manager_update_led_for_state(const char* state_name);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H

// Made with Bob
