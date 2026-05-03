/**
 * @file ble_service.c
 * @brief BLE GATT service implementation using NimBLE
 */

#include "ble_service.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include <string.h>

static const char *TAG = "BLE_SERVICE";

// Nordic UART Service UUIDs
#define BLE_UUID_NUS_SERVICE        0x0001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E
#define BLE_UUID_NUS_RX_CHAR        0x0002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E
#define BLE_UUID_NUS_TX_CHAR        0x0003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E
#define BLE_UUID_NUS_STATUS_CHAR    0x0004, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E

// Device name
#define DEVICE_NAME "DataLogger"
static char device_name[32] = DEVICE_NAME;

// BLE state
static ble_status_t ble_status = BLE_DISCONNECTED;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t tx_handle;
static uint16_t status_handle;

// Statistics
static ble_stats_t stats = {0};

// Callbacks
static ble_command_callback_t command_callback = NULL;
static ble_connection_callback_t connection_callback = NULL;

// Forward declarations
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int ble_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

/**
 * @brief Parse BLE command from received data
 */
static ble_command_t parse_command(const uint8_t *data, uint16_t len)
{
    if (len < 4) {
        return BLE_CMD_UNKNOWN;
    }
    
    if (memcmp(data, "START", 5) == 0) {
        return BLE_CMD_START;
    } else if (memcmp(data, "STOP", 4) == 0) {
        return BLE_CMD_STOP;
    } else if (memcmp(data, "STATUS", 6) == 0) {
        return BLE_CMD_STATUS;
    } else if (memcmp(data, "LIST", 4) == 0) {
        return BLE_CMD_LIST_FILES;
    } else if (memcmp(data, "DOWNLOAD", 8) == 0) {
        return BLE_CMD_DOWNLOAD;
    } else if (memcmp(data, "DELETE", 6) == 0) {
        return BLE_CMD_DELETE;
    } else if (memcmp(data, "CLEAR", 5) == 0) {
        return BLE_CMD_CLEAR;
    }
    
    return BLE_CMD_UNKNOWN;
}

/**
 * @brief GATT characteristic access callback
 */
static int ble_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGI(TAG, "GATT read characteristic");
            break;
            
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            ESP_LOGI(TAG, "GATT write characteristic, len=%d", ctxt->om->om_len);
            
            // Parse command
            ble_command_t cmd = parse_command(ctxt->om->om_data, ctxt->om->om_len);
            
            if (cmd != BLE_CMD_UNKNOWN) {
                stats.commands_received++;
                
                // Call command callback
                if (command_callback != NULL) {
                    command_callback(cmd, ctxt->om->om_data, ctxt->om->om_len);
                }
            } else {
                ESP_LOGW(TAG, "Unknown command received");
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unexpected GATT access operation: %d", ctxt->op);
            break;
    }
    
    return 0;
}

/**
 * @brief GATT service definition
 */
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        // Nordic UART Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID128_DECLARE(BLE_UUID_NUS_SERVICE),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // RX Characteristic (Write)
                .uuid = BLE_UUID128_DECLARE(BLE_UUID_NUS_RX_CHAR),
                .access_cb = ble_gatt_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                // TX Characteristic (Notify)
                .uuid = BLE_UUID128_DECLARE(BLE_UUID_NUS_TX_CHAR),
                .access_cb = ble_gatt_access_cb,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &tx_handle,
            },
            {
                // Status Characteristic (Read/Notify)
                .uuid = BLE_UUID128_DECLARE(BLE_UUID_NUS_STATUS_CHAR),
                .access_cb = ble_gatt_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &status_handle,
            },
            {
                0, // No more characteristics
            }
        },
    },
    {
        0, // No more services
    },
};

/**
 * @brief GAP event handler
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "BLE GAP event: Connect, status=%d", event->connect.status);
            
            if (event->connect.status == 0) {
                conn_handle = event->connect.conn_handle;
                ble_status = BLE_CONNECTED;
                stats.connected = true;
                stats.conn_handle = conn_handle;
                
                // Call connection callback
                if (connection_callback != NULL) {
                    connection_callback(true);
                }
            } else {
                // Connection failed, resume advertising
                ble_service_start_advertising();
            }
            break;
            
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE GAP event: Disconnect, reason=%d", event->disconnect.reason);
            
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_status = BLE_DISCONNECTED;
            stats.connected = false;
            
            // Call connection callback
            if (connection_callback != NULL) {
                connection_callback(false);
            }
            
            // Resume advertising
            ble_service_start_advertising();
            break;
            
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "BLE GAP event: Advertising complete");
            ble_service_start_advertising();
            break;
            
        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "BLE GAP event: Subscribe, handle=%d", event->subscribe.attr_handle);
            
            if (event->subscribe.cur_notify) {
                ble_status = BLE_STREAMING;
            }
            break;
            
        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "BLE GAP event: MTU update, mtu=%d", event->mtu.value);
            break;
            
        default:
            break;
    }
    
    return 0;
}

/**
 * @brief Start BLE advertising
 */
esp_err_t ble_service_start_advertising(void)
{
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};
    int rc;
    
    // Set advertising parameters
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    // Set advertising data
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set advertising data: %d", rc);
        return ESP_FAIL;
    }
    
    // Start advertising
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
        return ESP_FAIL;
    }
    
    ble_status = BLE_ADVERTISING;
    ESP_LOGI(TAG, "BLE advertising started");
    
    return ESP_OK;
}

/**
 * @brief Stop BLE advertising
 */
esp_err_t ble_service_stop_advertising(void)
{
    int rc = ble_gap_adv_stop();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to stop advertising: %d", rc);
        return ESP_FAIL;
    }
    
    ble_status = BLE_DISCONNECTED;
    ESP_LOGI(TAG, "BLE advertising stopped");
    
    return ESP_OK;
}

/**
 * @brief BLE host task
 */
static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * @brief On sync callback
 */
static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "BLE stack synchronized");
    
    // Start advertising
    ble_service_start_advertising();
}

/**
 * @brief On reset callback
 */
static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE stack reset, reason=%d", reason);
}

/**
 * @brief Initialize BLE service
 */
esp_err_t ble_service_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE service...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize NimBLE
    ESP_ERROR_CHECK(nimble_port_init());
    
    // Initialize GAP service
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Set device name
    ble_svc_gap_device_name_set(device_name);
    
    // Register GATT services
    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT services: %d", rc);
        return ESP_FAIL;
    }
    
    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
        return ESP_FAIL;
    }
    
    // Set callbacks
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;
    
    // Start BLE host task
    nimble_port_freertos_init(ble_host_task);
    
    ESP_LOGI(TAG, "BLE service initialized");
    return ESP_OK;
}

/**
 * @brief Deinitialize BLE service
 */
esp_err_t ble_service_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing BLE service...");
    
    // Stop advertising
    ble_service_stop_advertising();
    
    // Disconnect if connected
    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    
    // Deinitialize NimBLE
    nimble_port_deinit();
    
    ble_status = BLE_DISCONNECTED;
    ESP_LOGI(TAG, "BLE service deinitialized");
    
    return ESP_OK;
}

/**
 * @brief Send sensor sample via BLE
 */
esp_err_t ble_service_send_sample(const sensor_sample_t *sample)
{
    if (!stats.connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Send as binary data (50 bytes)
    struct os_mbuf *om = ble_hs_mbuf_from_flat(sample, sizeof(sensor_sample_t));
    if (om == NULL) {
        stats.packets_failed++;
        return ESP_ERR_NO_MEM;
    }
    
    int rc = ble_gattc_notify_custom(conn_handle, tx_handle, om);
    if (rc != 0) {
        stats.packets_failed++;
        return ESP_FAIL;
    }
    
    stats.packets_sent++;
    stats.bytes_sent += sizeof(sensor_sample_t);
    
    return ESP_OK;
}

/**
 * @brief Send multiple samples
 */
uint32_t ble_service_send_samples(const sensor_sample_t *samples, uint32_t count)
{
    uint32_t sent = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        if (ble_service_send_sample(&samples[i]) == ESP_OK) {
            sent++;
        } else {
            break;
        }
        
        // Small delay to avoid overwhelming BLE stack
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    return sent;
}

/**
 * @brief Send status message
 */
esp_err_t ble_service_send_status(const char *status_json)
{
    return ble_service_send_message(status_json);
}

/**
 * @brief Send text message
 */
esp_err_t ble_service_send_message(const char *message)
{
    if (message == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return ble_service_send_data((const uint8_t *)message, strlen(message));
}

/**
 * @brief Send binary data
 */
esp_err_t ble_service_send_data(const uint8_t *data, uint16_t len)
{
    if (!stats.connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        stats.packets_failed++;
        return ESP_ERR_NO_MEM;
    }
    
    int rc = ble_gattc_notify_custom(conn_handle, tx_handle, om);
    if (rc != 0) {
        stats.packets_failed++;
        return ESP_FAIL;
    }
    
    stats.packets_sent++;
    stats.bytes_sent += len;
    
    return ESP_OK;
}

/**
 * @brief Get BLE status
 */
ble_status_t ble_service_get_status(void)
{
    return ble_status;
}

/**
 * @brief Check if connected
 */
bool ble_service_is_connected(void)
{
    return stats.connected;
}

/**
 * @brief Get statistics
 */
ble_stats_t ble_service_get_stats(void)
{
    return stats;
}

/**
 * @brief Register command callback
 */
esp_err_t ble_service_register_command_callback(ble_command_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    command_callback = callback;
    ESP_LOGI(TAG, "Command callback registered");
    
    return ESP_OK;
}

/**
 * @brief Register connection callback
 */
esp_err_t ble_service_register_connection_callback(ble_connection_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    connection_callback = callback;
    ESP_LOGI(TAG, "Connection callback registered");
    
    return ESP_OK;
}

/**
 * @brief Get device name
 */
const char* ble_service_get_device_name(void)
{
    return device_name;
}

/**
 * @brief Set device name
 */
esp_err_t ble_service_set_device_name(const char *name)
{
    if (name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(device_name, name, sizeof(device_name) - 1);
    device_name[sizeof(device_name) - 1] = '\0';
    
    ble_svc_gap_device_name_set(device_name);
    
    ESP_LOGI(TAG, "Device name set to: %s", device_name);
    return ESP_OK;
}

/**
 * @brief Disconnect client
 */
esp_err_t ble_service_disconnect(void)
{
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to disconnect: %d", rc);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Made with Bob
