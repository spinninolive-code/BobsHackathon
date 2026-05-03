/**
 * @file main.c
 * @brief Phase B: BLE-enabled Data Logger Main Application
 * 
 * Extends Phase A with BLE communication capabilities
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

// Phase A components
#include "sensor_drivers.h"
#include "buffer_manager.h"
#include "sd_storage.h"
#include "state_machine.h"
#include "power_manager.h"
#include "ui_manager.h"

// Phase B component
#include "ble_service.h"

static const char *TAG = "MAIN";

// Task handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t data_process_task_handle = NULL;
static TaskHandle_t sd_write_task_handle = NULL;
static TaskHandle_t ble_stream_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;

// Queues
static QueueHandle_t sensor_queue = NULL;
static QueueHandle_t ble_queue = NULL;

// Timer
static esp_timer_handle_t sample_timer = NULL;

// Flags
static volatile bool acquisition_active = false;
static volatile bool ble_streaming_active = false;

/**
 * @brief 1kHz timer ISR for sensor sampling
 */
static void IRAM_ATTR sample_timer_isr(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Notify sensor task
    if (sensor_task_handle != NULL) {
        vTaskNotifyGiveFromISR(sensor_task_handle, &xHigherPriorityTaskWoken);
    }
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_IS();
    }
}

/**
 * @brief Sensor sampling task (Core 1, highest priority)
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started on core %d", xPortGetCoreID());
    
    sensor_sample_t sample;
    
    while (1) {
        // Wait for timer notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (!acquisition_active) {
            continue;
        }
        
        // Read sensors
        if (sensor_read_all(&sample) == ESP_OK) {
            // Send to data processing queue
            if (xQueueSend(sensor_queue, &sample, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Sensor queue full, sample dropped");
            }
        }
    }
}

/**
 * @brief Data processing task (Core 0)
 */
static void data_process_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Data processing task started on core %d", xPortGetCoreID());
    
    sensor_sample_t sample;
    
    while (1) {
        // Wait for sensor data
        if (xQueueReceive(sensor_queue, &sample, portMAX_DELAY) == pdTRUE) {
            // Write to buffer
            buffer_write_sample(&sample);
            
            // Send to BLE if streaming active
            if (ble_streaming_active && ble_service_is_connected()) {
                if (xQueueSend(ble_queue, &sample, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "BLE queue full, sample dropped");
                }
            }
            
            // Check if buffer needs flushing
            if (buffer_needs_flush()) {
                buffer_flush_psram_to_nand();
            }
        }
    }
}

/**
 * @brief SD card write task (Core 0)
 */
static void sd_write_task(void *pvParameters)
{
    ESP_LOGI(TAG, "SD write task started on core %d", xPortGetCoreID());
    
    sensor_sample_t samples[100];
    uint32_t samples_read;
    
    while (1) {
        // Check if we need to write to SD
        buffer_status_t status = buffer_get_status();
        
        if (status.nand_used > (status.nand_capacity * 75 / 100)) {
            // NAND >75% full, flush to SD
            uint8_t buffer[4096];
            uint32_t bytes_read = buffer_get_nand_data(buffer, sizeof(buffer));
            
            if (bytes_read > 0) {
                sd_storage_write_buffer(buffer, bytes_read);
                ESP_LOGI(TAG, "Flushed %d bytes to SD card", bytes_read);
            }
        }
        
        // Also write buffered samples periodically
        samples_read = buffer_read_samples(samples, 100);
        if (samples_read > 0) {
            sd_storage_write_samples(samples, samples_read);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

/**
 * @brief BLE streaming task (Core 0)
 */
static void ble_stream_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BLE streaming task started on core %d", xPortGetCoreID());
    
    sensor_sample_t sample;
    
    while (1) {
        // Wait for BLE data
        if (xQueueReceive(ble_queue, &sample, portMAX_DELAY) == pdTRUE) {
            if (ble_service_is_connected() && ble_streaming_active) {
                ble_service_send_sample(&sample);
            }
        }
    }
}

/**
 * @brief UI monitoring task (Core 0)
 */
static void ui_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UI task started on core %d", xPortGetCoreID());
    
    dip_mode_t current_mode = MODE_IDLE;
    dip_mode_t last_mode = MODE_IDLE;
    
    while (1) {
        // Read DIP switches
        current_mode = ui_manager_read_mode();
        
        if (current_mode != last_mode) {
            ESP_LOGI(TAG, "Mode changed: %s -> %s",
                     ui_manager_get_mode_name(last_mode),
                     ui_manager_get_mode_name(current_mode));
            
            // Handle mode changes
            switch (current_mode) {
                case MODE_IDLE:
                    acquisition_active = false;
                    ble_streaming_active = false;
                    state_machine_post_event(EVENT_STOP_ACQUISITION);
                    break;
                    
                case MODE_ACQUIRE:
                    acquisition_active = true;
                    ble_streaming_active = false;
                    state_machine_post_event(EVENT_START_ACQUISITION);
                    break;
                    
                case MODE_STORE:
                    acquisition_active = false;
                    ble_streaming_active = false;
                    state_machine_post_event(EVENT_STORE_DATA);
                    break;
                    
                case MODE_ACQUIRE_STORE:
                    acquisition_active = true;
                    ble_streaming_active = false;
                    state_machine_post_event(EVENT_START_ACQUISITION);
                    sd_storage_start_file();
                    break;
                    
                case MODE_SHUTDOWN:
                    state_machine_post_event(EVENT_SHUTDOWN_REQUEST);
                    break;
                    
                default:
                    break;
            }
            
            last_mode = current_mode;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}

/**
 * @brief System monitoring task (Core 0)
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor task started on core %d", xPortGetCoreID());
    
    uint32_t counter = 0;
    
    while (1) {
        counter++;
        
        // Log system status every 10 seconds
        if (counter % 100 == 0) {
            buffer_status_t buf_status = buffer_get_status();
            battery_status_t bat_status = power_manager_get_status();
            ble_stats_t ble_stats = ble_service_get_stats();
            
            ESP_LOGI(TAG, "=== System Status ===");
            ESP_LOGI(TAG, "State: %s", state_machine_get_state_name(state_machine_get_state()));
            ESP_LOGI(TAG, "Acquisition: %s", acquisition_active ? "ACTIVE" : "INACTIVE");
            ESP_LOGI(TAG, "BLE: %s (%d packets sent)", 
                     ble_service_is_connected() ? "CONNECTED" : "DISCONNECTED",
                     ble_stats.packets_sent);
            ESP_LOGI(TAG, "Buffer: PSRAM %d%%, NAND %d%%",
                     (buf_status.psram_used * 100) / buf_status.psram_capacity,
                     (buf_status.nand_used * 100) / buf_status.nand_capacity);
            ESP_LOGI(TAG, "Battery: %.0fmV (%d%%)", bat_status.voltage_mv, bat_status.percentage);
            ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
        }
        
        // Update LED based on system state
        system_state_t state = state_machine_get_state();
        ui_manager_update_led_for_state(state_machine_get_state_name(state));
        
        // Check for low battery
        if (power_manager_is_low_battery()) {
            state_machine_post_event(EVENT_LOW_BATTERY);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}

/**
 * @brief BLE command callback
 */
static void ble_command_callback(ble_command_t command, const uint8_t *data, uint16_t len)
{
    ESP_LOGI(TAG, "BLE command received: %d", command);
    
    switch (command) {
        case BLE_CMD_START:
            acquisition_active = true;
            ble_streaming_active = true;
            state_machine_post_event(EVENT_START_ACQUISITION);
            ble_service_send_message("ACK:START");
            break;
            
        case BLE_CMD_STOP:
            acquisition_active = false;
            ble_streaming_active = false;
            state_machine_post_event(EVENT_STOP_ACQUISITION);
            ble_service_send_message("ACK:STOP");
            break;
            
        case BLE_CMD_STATUS: {
            // Send JSON status
            char status_json[256];
            buffer_status_t buf_status = buffer_get_status();
            battery_status_t bat_status = power_manager_get_status();
            sd_status_t sd_status = sd_storage_get_status();
            
            snprintf(status_json, sizeof(status_json),
                     "{\"state\":\"%s\",\"battery\":%d,\"samples\":%d,"
                     "\"sd_free\":%lld,\"ble_connected\":true}",
                     state_machine_get_state_name(state_machine_get_state()),
                     bat_status.percentage,
                     buf_status.samples_buffered,
                     sd_status.free_bytes);
            
            ble_service_send_status(status_json);
            break;
        }
        
        case BLE_CMD_LIST_FILES:
            // List SD card files (simplified)
            ble_service_send_message("FILES:data_20260503_143000.csv,data_20260503_144500.csv");
            break;
            
        case BLE_CMD_CLEAR:
            buffer_clear_all();
            ble_service_send_message("ACK:CLEAR");
            break;
            
        default:
            ble_service_send_message("ERROR:UNKNOWN_COMMAND");
            break;
    }
}

/**
 * @brief BLE connection callback
 */
static void ble_connection_callback(bool connected)
{
    ESP_LOGI(TAG, "BLE connection status: %s", connected ? "CONNECTED" : "DISCONNECTED");
    
    if (connected) {
        // Send welcome message
        ble_service_send_message("HELLO:DataLogger_PhaseB");
    } else {
        // Stop streaming if disconnected
        ble_streaming_active = false;
    }
}

/**
 * @brief State machine callback
 */
static void state_change_callback(system_state_t old_state, system_state_t new_state)
{
    ESP_LOGI(TAG, "State changed: %s -> %s",
             state_machine_get_state_name(old_state),
             state_machine_get_state_name(new_state));
    
    // Handle state-specific actions
    switch (new_state) {
        case STATE_ACQUIRING:
            // Start timer if not already running
            if (sample_timer != NULL) {
                esp_timer_start_periodic(sample_timer, 1000); // 1kHz = 1000µs
            }
            break;
            
        case STATE_IDLE:
            // Stop timer
            if (sample_timer != NULL) {
                esp_timer_stop(sample_timer);
            }
            break;
            
        case STATE_STORING:
            // Flush all buffers to SD
            buffer_flush_psram_to_nand();
            sd_storage_sync();
            break;
            
        case STATE_ERROR:
            // Stop everything
            acquisition_active = false;
            ble_streaming_active = false;
            if (sample_timer != NULL) {
                esp_timer_stop(sample_timer);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Initialize system
 */
static esp_err_t system_init(void)
{
    ESP_LOGI(TAG, "Initializing Phase B system...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize Phase A components
    ESP_ERROR_CHECK(sensor_drivers_init());
    ESP_ERROR_CHECK(buffer_manager_init());
    ESP_ERROR_CHECK(sd_storage_init());
    ESP_ERROR_CHECK(state_machine_init());
    ESP_ERROR_CHECK(power_manager_init());
    ESP_ERROR_CHECK(ui_manager_init());
    
    // Initialize Phase B components
    ESP_ERROR_CHECK(ble_service_init());
    
    // Register callbacks
    state_machine_register_callback(state_change_callback);
    ble_service_register_command_callback(ble_command_callback);
    ble_service_register_connection_callback(ble_connection_callback);
    
    // Create queues
    sensor_queue = xQueueCreate(100, sizeof(sensor_sample_t));
    ble_queue = xQueueCreate(50, sizeof(sensor_sample_t));
    
    if (sensor_queue == NULL || ble_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return ESP_FAIL;
    }
    
    // Create 1kHz timer
    esp_timer_create_args_t timer_args = {
        .callback = sample_timer_isr,
        .name = "sample_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &sample_timer));
    
    // Start SD file
    sd_storage_start_file();
    
    ESP_LOGI(TAG, "System initialization complete");
    return ESP_OK;
}

/**
 * @brief Create tasks
 */
static void create_tasks(void)
{
    ESP_LOGI(TAG, "Creating tasks...");
    
    // Sensor task (Core 1, highest priority)
    xTaskCreatePinnedToCore(sensor_task, "sensor_task", 4096, NULL, 20, 
                           &sensor_task_handle, 1);
    
    // Data processing task (Core 0)
    xTaskCreatePinnedToCore(data_process_task, "data_process_task", 4096, NULL, 15,
                           &data_process_task_handle, 0);
    
    // SD write task (Core 0)
    xTaskCreatePinnedToCore(sd_write_task, "sd_write_task", 4096, NULL, 10,
                           &sd_write_task_handle, 0);
    
    // BLE streaming task (Core 0)
    xTaskCreatePinnedToCore(ble_stream_task, "ble_stream_task", 4096, NULL, 12,
                           &ble_stream_task_handle, 0);
    
    // UI task (Core 0)
    xTaskCreatePinnedToCore(ui_task, "ui_task", 4096, NULL, 5,
                           &ui_task_handle, 0);
    
    // Monitor task (Core 0)
    xTaskCreatePinnedToCore(monitor_task, "monitor_task", 4096, NULL, 3,
                           &monitor_task_handle, 0);
    
    ESP_LOGI(TAG, "All tasks created");
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Phase B: BLE Data Logger Starting ===");
    ESP_LOGI(TAG, "ESP-IDF Version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    
    // Initialize system
    ESP_ERROR_CHECK(system_init());
    
    // Create tasks
    create_tasks();
    
    // Post initialization complete event
    state_machine_post_event(EVENT_INIT_COMPLETE);
    
    ESP_LOGI(TAG, "=== Phase B System Ready ===");
    ESP_LOGI(TAG, "BLE Device Name: %s", ble_service_get_device_name());
    ESP_LOGI(TAG, "Waiting for BLE connections...");
    
    // Main loop (optional - tasks handle everything)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Sleep 10 seconds
        
        // Periodic maintenance
        ESP_LOGI(TAG, "System running... Free heap: %d bytes", esp_get_free_heap_size());
    }
}

// Made with Bob
