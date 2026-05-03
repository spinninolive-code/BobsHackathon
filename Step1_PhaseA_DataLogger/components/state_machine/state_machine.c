/**
 * @file state_machine.c
 * @brief System state machine implementation
 */

#include "state_machine.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "STATE_MACHINE";

// Current state
static system_state_t current_state = STATE_INIT;
static SemaphoreHandle_t state_mutex = NULL;

// State change callback
static state_change_callback_t state_callback = NULL;

// State transition table
static const bool transition_table[7][7] = {
    // TO:  INIT  IDLE  ACQ   STOR  ERR   LOW   SHUT
    /* INIT */  {true,  true,  false, false, true,  false, true},
    /* IDLE */  {false, true,  true,  true,  true,  true,  true},
    /* ACQ  */  {false, true,  true,  true,  true,  true,  true},
    /* STOR */  {false, true,  false, true,  true,  true,  true},
    /* ERR  */  {false, true,  false, false, true,  false, true},
    /* LOW  */  {false, false, false, false, true,  true,  true},
    /* SHUT */  {false, false, false, false, false, false, true}
};

/**
 * @brief Initialize state machine
 */
esp_err_t state_machine_init(void)
{
    ESP_LOGI(TAG, "Initializing state machine...");
    
    // Create mutex
    state_mutex = xSemaphoreCreateMutex();
    if (state_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create state mutex");
        return ESP_FAIL;
    }
    
    current_state = STATE_INIT;
    
    ESP_LOGI(TAG, "State machine initialized in %s state",
             state_machine_get_state_name(current_state));
    
    return ESP_OK;
}

/**
 * @brief Get current system state
 */
system_state_t state_machine_get_state(void)
{
    system_state_t state;
    
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    state = current_state;
    xSemaphoreGive(state_mutex);
    
    return state;
}

/**
 * @brief Transition to new state
 */
static esp_err_t transition_to_state(system_state_t new_state)
{
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    
    // Check if transition is valid
    if (!state_machine_is_valid_transition(current_state, new_state)) {
        ESP_LOGW(TAG, "Invalid transition from %s to %s",
                 state_machine_get_state_name(current_state),
                 state_machine_get_state_name(new_state));
        xSemaphoreGive(state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    system_state_t old_state = current_state;
    current_state = new_state;
    
    ESP_LOGI(TAG, "State transition: %s -> %s",
             state_machine_get_state_name(old_state),
             state_machine_get_state_name(new_state));
    
    // Call callback if registered
    if (state_callback != NULL) {
        state_callback(old_state, new_state);
    }
    
    xSemaphoreGive(state_mutex);
    return ESP_OK;
}

/**
 * @brief Post event to state machine
 */
esp_err_t state_machine_post_event(system_event_t event)
{
    ESP_LOGI(TAG, "Event posted: %s (current state: %s)",
             state_machine_get_event_name(event),
             state_machine_get_state_name(current_state));
    
    system_state_t new_state = current_state;
    
    // Determine new state based on current state and event
    switch (current_state) {
        case STATE_INIT:
            if (event == EVENT_INIT_COMPLETE) {
                new_state = STATE_IDLE;
            } else if (event == EVENT_ERROR_OCCURRED) {
                new_state = STATE_ERROR;
            }
            break;
            
        case STATE_IDLE:
            if (event == EVENT_START_ACQUISITION) {
                new_state = STATE_ACQUIRING;
            } else if (event == EVENT_STORE_DATA) {
                new_state = STATE_STORING;
            } else if (event == EVENT_ERROR_OCCURRED) {
                new_state = STATE_ERROR;
            } else if (event == EVENT_LOW_BATTERY) {
                new_state = STATE_LOW_BATTERY;
            } else if (event == EVENT_SHUTDOWN_REQUEST) {
                new_state = STATE_SHUTDOWN;
            }
            break;
            
        case STATE_ACQUIRING:
            if (event == EVENT_STOP_ACQUISITION) {
                new_state = STATE_IDLE;
            } else if (event == EVENT_STORE_DATA) {
                new_state = STATE_STORING;
            } else if (event == EVENT_ERROR_OCCURRED) {
                new_state = STATE_ERROR;
            } else if (event == EVENT_LOW_BATTERY) {
                new_state = STATE_LOW_BATTERY;
            } else if (event == EVENT_SHUTDOWN_REQUEST) {
                new_state = STATE_SHUTDOWN;
            }
            break;
            
        case STATE_STORING:
            if (event == EVENT_STOP_ACQUISITION) {
                new_state = STATE_IDLE;
            } else if (event == EVENT_ERROR_OCCURRED) {
                new_state = STATE_ERROR;
            } else if (event == EVENT_LOW_BATTERY) {
                new_state = STATE_LOW_BATTERY;
            } else if (event == EVENT_SHUTDOWN_REQUEST) {
                new_state = STATE_SHUTDOWN;
            }
            break;
            
        case STATE_ERROR:
            if (event == EVENT_RECOVERY) {
                new_state = STATE_IDLE;
            } else if (event == EVENT_SHUTDOWN_REQUEST) {
                new_state = STATE_SHUTDOWN;
            }
            break;
            
        case STATE_LOW_BATTERY:
            if (event == EVENT_SHUTDOWN_REQUEST) {
                new_state = STATE_SHUTDOWN;
            } else if (event == EVENT_ERROR_OCCURRED) {
                new_state = STATE_ERROR;
            }
            break;
            
        case STATE_SHUTDOWN:
            // No transitions from shutdown
            break;
    }
    
    // Transition to new state if changed
    if (new_state != current_state) {
        return transition_to_state(new_state);
    }
    
    return ESP_OK;
}

/**
 * @brief Register state change callback
 */
esp_err_t state_machine_register_callback(state_change_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    state_callback = callback;
    ESP_LOGI(TAG, "State change callback registered");
    
    return ESP_OK;
}

/**
 * @brief Get state name string
 */
const char* state_machine_get_state_name(system_state_t state)
{
    switch (state) {
        case STATE_INIT:        return "INIT";
        case STATE_IDLE:        return "IDLE";
        case STATE_ACQUIRING:   return "ACQUIRING";
        case STATE_STORING:     return "STORING";
        case STATE_ERROR:       return "ERROR";
        case STATE_LOW_BATTERY: return "LOW_BATTERY";
        case STATE_SHUTDOWN:    return "SHUTDOWN";
        default:                return "UNKNOWN";
    }
}

/**
 * @brief Get event name string
 */
const char* state_machine_get_event_name(system_event_t event)
{
    switch (event) {
        case EVENT_INIT_COMPLETE:     return "INIT_COMPLETE";
        case EVENT_START_ACQUISITION: return "START_ACQUISITION";
        case EVENT_STOP_ACQUISITION:  return "STOP_ACQUISITION";
        case EVENT_STORE_DATA:        return "STORE_DATA";
        case EVENT_ERROR_OCCURRED:    return "ERROR_OCCURRED";
        case EVENT_LOW_BATTERY:       return "LOW_BATTERY";
        case EVENT_SHUTDOWN_REQUEST:  return "SHUTDOWN_REQUEST";
        case EVENT_RECOVERY:          return "RECOVERY";
        default:                      return "UNKNOWN";
    }
}

/**
 * @brief Check if state transition is valid
 */
bool state_machine_is_valid_transition(system_state_t from_state, system_state_t to_state)
{
    if (from_state >= 7 || to_state >= 7) {
        return false;
    }
    
    return transition_table[from_state][to_state];
}

// Made with Bob
