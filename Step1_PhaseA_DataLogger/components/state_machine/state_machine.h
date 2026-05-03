/**
 * @file state_machine.h
 * @brief System state machine interface
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System states
 */
typedef enum {
    STATE_INIT,              ///< Initialization state
    STATE_IDLE,              ///< Idle state (waiting for command)
    STATE_ACQUIRING,         ///< Acquiring sensor data
    STATE_STORING,           ///< Storing data to SD card
    STATE_ERROR,             ///< Error state
    STATE_LOW_BATTERY,       ///< Low battery state
    STATE_SHUTDOWN           ///< Shutdown state
} system_state_t;

/**
 * @brief System events
 */
typedef enum {
    EVENT_INIT_COMPLETE,     ///< Initialization complete
    EVENT_START_ACQUISITION, ///< Start data acquisition
    EVENT_STOP_ACQUISITION,  ///< Stop data acquisition
    EVENT_STORE_DATA,        ///< Store data to SD card
    EVENT_ERROR_OCCURRED,    ///< Error occurred
    EVENT_LOW_BATTERY,       ///< Battery low
    EVENT_SHUTDOWN_REQUEST,  ///< Shutdown requested
    EVENT_RECOVERY           ///< Recovery from error
} system_event_t;

/**
 * @brief State change callback function type
 * 
 * @param old_state Previous state
 * @param new_state New state
 */
typedef void (*state_change_callback_t)(system_state_t old_state, system_state_t new_state);

/**
 * @brief Initialize state machine
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t state_machine_init(void);

/**
 * @brief Get current system state
 * 
 * @return Current system state
 */
system_state_t state_machine_get_state(void);

/**
 * @brief Post event to state machine
 * 
 * Triggers state transition based on current state and event
 * 
 * @param event Event to post
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t state_machine_post_event(system_event_t event);

/**
 * @brief Register state change callback
 * 
 * @param callback Callback function to register
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t state_machine_register_callback(state_change_callback_t callback);

/**
 * @brief Get state name string
 * 
 * @param state State to get name for
 * @return State name string
 */
const char* state_machine_get_state_name(system_state_t state);

/**
 * @brief Get event name string
 * 
 * @param event Event to get name for
 * @return Event name string
 */
const char* state_machine_get_event_name(system_event_t event);

/**
 * @brief Check if state transition is valid
 * 
 * @param from_state Source state
 * @param to_state Destination state
 * @return true if transition is valid
 */
bool state_machine_is_valid_transition(system_state_t from_state, system_state_t to_state);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H

// Made with Bob
