/**
 * @file ble_service.h
 * @brief BLE GATT service interface for sensor data streaming
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sensor_drivers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE connection status
 */
typedef enum {
    BLE_DISCONNECTED,       ///< Not connected
    BLE_ADVERTISING,        ///< Advertising
    BLE_CONNECTED,          ///< Connected to client
    BLE_STREAMING           ///< Streaming data
} ble_status_t;

/**
 * @brief BLE command enumeration
 */
typedef enum {
    BLE_CMD_START,          ///< Start acquisition
    BLE_CMD_STOP,           ///< Stop acquisition
    BLE_CMD_STATUS,         ///< Request status
    BLE_CMD_LIST_FILES,     ///< List SD card files
    BLE_CMD_DOWNLOAD,       ///< Download file
    BLE_CMD_DELETE,         ///< Delete file
    BLE_CMD_CLEAR,          ///< Clear buffers
    BLE_CMD_UNKNOWN         ///< Unknown command
} ble_command_t;

/**
 * @brief BLE statistics
 */
typedef struct {
    uint32_t packets_sent;      ///< Total packets sent
    uint32_t packets_failed;    ///< Failed packet transmissions
    uint32_t bytes_sent;        ///< Total bytes sent
    uint32_t commands_received; ///< Commands received
    bool connected;             ///< Connection status
    uint16_t conn_handle;       ///< Connection handle
} ble_stats_t;

/**
 * @brief BLE command callback function type
 * 
 * @param command Command received
 * @param data Command data (if any)
 * @param len Data length
 */
typedef void (*ble_command_callback_t)(ble_command_t command, const uint8_t *data, uint16_t len);

/**
 * @brief BLE connection callback function type
 * 
 * @param connected Connection status
 */
typedef void (*ble_connection_callback_t)(bool connected);

/**
 * @brief Initialize BLE service
 * 
 * Initializes BLE stack, creates GATT service, starts advertising
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_init(void);

/**
 * @brief Deinitialize BLE service
 * 
 * Stops advertising, disconnects clients, deinitializes BLE stack
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_deinit(void);

/**
 * @brief Start BLE advertising
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_start_advertising(void);

/**
 * @brief Stop BLE advertising
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_stop_advertising(void);

/**
 * @brief Send sensor sample via BLE
 * 
 * Sends sensor data packet to connected client
 * 
 * @param sample Sensor sample to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_send_sample(const sensor_sample_t *sample);

/**
 * @brief Send multiple sensor samples via BLE
 * 
 * Sends batch of sensor samples to connected client
 * 
 * @param samples Array of sensor samples
 * @param count Number of samples
 * @return Number of samples successfully sent
 */
uint32_t ble_service_send_samples(const sensor_sample_t *samples, uint32_t count);

/**
 * @brief Send status message via BLE
 * 
 * Sends JSON status message to connected client
 * 
 * @param status_json JSON status string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_send_status(const char *status_json);

/**
 * @brief Send text message via BLE
 * 
 * Sends text message (ACK, ERROR, etc.) to connected client
 * 
 * @param message Text message
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_send_message(const char *message);

/**
 * @brief Send binary data via BLE
 * 
 * Sends raw binary data to connected client
 * 
 * @param data Binary data
 * @param len Data length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_send_data(const uint8_t *data, uint16_t len);

/**
 * @brief Get BLE connection status
 * 
 * @return Current BLE status
 */
ble_status_t ble_service_get_status(void);

/**
 * @brief Check if BLE is connected
 * 
 * @return true if connected to client
 */
bool ble_service_is_connected(void);

/**
 * @brief Get BLE statistics
 * 
 * @return BLE statistics structure
 */
ble_stats_t ble_service_get_stats(void);

/**
 * @brief Register command callback
 * 
 * @param callback Callback function for received commands
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_register_command_callback(ble_command_callback_t callback);

/**
 * @brief Register connection callback
 * 
 * @param callback Callback function for connection events
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_register_connection_callback(ble_connection_callback_t callback);

/**
 * @brief Get device name
 * 
 * @return BLE device name string
 */
const char* ble_service_get_device_name(void);

/**
 * @brief Set device name
 * 
 * @param name New device name
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_set_device_name(const char *name);

/**
 * @brief Disconnect current client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_service_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_SERVICE_H

// Made with Bob
