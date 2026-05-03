/**
 * @file main.c
 * @brief Main application for Phase A Data Logger
 * 
 * This is the entry point for the ESP32-S3 data logger application.
 * It initializes all subsystems and starts the FreeRTOS tasks.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "sensor_drivers.h"
#include "buffer_manager.h"
#include "sd_storage.h"
#include "state_machine.h"
#include "power_manager.h"
#include "ui_manager.h"

static const char *TAG = LOG_TAG_MAIN;

// Global handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t data_process_task_handle = NULL;
static TaskHandle_t sd_write_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;

// Timer for sensor sampling
static esp_timer_handle_t sensor_timer = NULL;

// Queues for inter-task communication
static QueueHandle_t sensor_data_queue = NULL;
static QueueHandle_t sd_write_queue = NULL;

// Synchronization
static SemaphoreHandle_t spi_mutex = NULL;

/**
 * @brief Sensor timer callback (1kHz ISR)
 * 
 * This function is called every 1ms by the high-resolution timer.
 * It triggers sensor reading in the sensor task.
 */
static void IRAM_ATTR sensor_timer_callback(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Notify sensor task to read sensors
    if (sensor_task_handle != NULL) {
        vTaskNotifyGiveFromISR(sensor_task_handle, &xHigherPriorityTaskWoken);
        
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

/**
 * @brief Sensor task
 * 
 * Reads all sensors at 1kHz and writes data to PSRAM buffer.
 * This is the highest priority task for time-critical operations.
 */
static void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started");
    
    sensor_sample_t sample;
    uint32_t notification_value;
    
    while (1) {
        // Wait for timer notification
        notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (notification_value > 0) {
            // Get timestamp
            sample.timestamp_us = esp_timer_get_time();
            
            // Read Module 1 sensors
            sample.module_id = 0;
            if (sensor_read_all_module(0, &sample) == ESP_OK) {
                // Write to buffer
                buffer_write_sample(&sample);
            }
            
            // Read Module 2 sensors
            sample.module_id = 1;
            if (sensor_read_all_module(1, &sample) == ESP_OK) {
                // Write to buffer
                buffer_write_sample(&sample);
            }
        }
    }
}

/**
 * @brief Data processing task
 * 
 * Monitors buffer levels and triggers transfers between
 * PSRAM → NAND → SD card as needed.
 */
static void data_process_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Data processing task started");
    
    buffer_status_t status;
    TickType_t last_check = xTaskGetTickCount();
    
    while (1) {
        // Check buffer status every 100ms
        vTaskDelayUntil(&last_check, pdMS_TO_TICKS(100));
        
        status = buffer_get_status();
        
        // Check if PSRAM needs flushing to NAND
        if (status.psram_used >= PSRAM_FLUSH_THRESHOLD) {
            ESP_LOGW(TAG, "PSRAM buffer at %d%%, flushing to NAND",
                     (status.psram_used * 100) / status.psram_capacity);
            buffer_flush_psram_to_nand();
        }
        
        // Check if NAND needs flushing to SD
        if (status.nand_used >= FLASH_FLUSH_THRESHOLD) {
            ESP_LOGW(TAG, "NAND cache at %d%%, flushing to SD",
                     (status.nand_used * 100) / status.nand_capacity);
            buffer_flush_nand_to_sd();
        }
        
        // Check for overflow warning
        if (status.overflow_warning) {
            ESP_LOGE(TAG, "Buffer overflow warning!");
            ui_set_error_state(true);
        }
    }
}

/**
 * @brief SD card write task
 * 
 * Handles writing data to SD card in large chunks
 * for optimal performance.
 */
static void sd_write_task(void *pvParameters)
{
    ESP_LOGI(TAG, "SD write task started");
    
    while (1) {
        // Wait for data to write
        // This task is triggered by the data processing task
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Check if we need to write
        if (sd_needs_write()) {
            sd_write_buffered_data();
        }
    }
}

/**
 * @brief UI task
 * 
 * Monitors DIP switches and updates LED status.
 * Handles mode changes and user interface.
 */
static void ui_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UI task started");
    
    uint8_t current_mode = 0;
    uint8_t new_mode;
    system_state_t state;
    
    while (1) {
        // Read DIP switches
        new_mode = ui_read_dip_switches();
        
        // Check for mode change
        if (new_mode != current_mode) {
            ESP_LOGI(TAG, "Mode changed: %d -> %d", current_mode, new_mode);
            current_mode = new_mode;
            
            // Process mode change
            state_machine_process_mode_change(new_mode);
        }
        
        // Update LED based on current state
        state = state_machine_get_state();
        ui_update_led(state);
        
        // Check every 100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Monitor task
 * 
 * Monitors battery voltage and system health.
 * Lowest priority background task.
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor task started");
    
    uint16_t battery_mv;
    uint8_t battery_percent;
    battery_status_t battery_status;
    
    while (1) {
        // Read battery voltage
        battery_mv = power_read_battery_mv();
        battery_percent = power_get_battery_percent();
        battery_status = power_get_battery_status();
        
        // Log battery status every 60 seconds
        ESP_LOGI(TAG, "Battery: %dmV (%d%%) - Status: %d", 
                 battery_mv, battery_percent, battery_status);
        
        // Check for low battery
        if (battery_status == BATTERY_CRITICAL) {
            ESP_LOGE(TAG, "Critical battery level! Initiating shutdown...");
            state_machine_process_event(EVENT_BATTERY_LOW);
        } else if (battery_status == BATTERY_LOW) {
            ESP_LOGW(TAG, "Low battery warning");
        }
        
        // Check every 10 seconds
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * @brief Initialize all subsystems
 */
static esp_err_t init_subsystems(void)
{
    esp_err_t ret;
    
    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Create SPI mutex
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create SPI mutex");
        return ESP_FAIL;
    }
    
    // Initialize UI manager (DIP switches, LED)
    ESP_LOGI(TAG, "Initializing UI manager...");
    ret = ui_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI manager");
        return ret;
    }
    
    // Initialize power manager
    ESP_LOGI(TAG, "Initializing power manager...");
    ret = power_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize power manager");
        return ret;
    }
    
    // Initialize sensor drivers
    ESP_LOGI(TAG, "Initializing sensor drivers...");
    ret = sensor_drivers_init(spi_mutex);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor drivers");
        return ret;
    }
    
    // Initialize buffer manager
    ESP_LOGI(TAG, "Initializing buffer manager...");
    ret = buffer_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize buffer manager");
        return ret;
    }
    
    // Initialize SD storage
    ESP_LOGI(TAG, "Initializing SD storage...");
    ret = sd_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD storage");
        return ret;
    }
    
    // Initialize state machine
    ESP_LOGI(TAG, "Initializing state machine...");
    ret = state_machine_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize state machine");
        return ret;
    }
    
    ESP_LOGI(TAG, "All subsystems initialized successfully");
    return ESP_OK;
}

/**
 * @brief Create all FreeRTOS tasks
 */
static esp_err_t create_tasks(void)
{
    BaseType_t ret;
    
    // Create sensor task (highest priority)
    ret = xTaskCreatePinnedToCore(
        sensor_task,
        "sensor_task",
        STACK_SIZE_SENSOR,
        NULL,
        TASK_PRIORITY_SENSOR,
        &sensor_task_handle,
        1  // Pin to core 1 (application CPU)
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task");
        return ESP_FAIL;
    }
    
    // Create data processing task
    ret = xTaskCreatePinnedToCore(
        data_process_task,
        "data_process",
        STACK_SIZE_DATA,
        NULL,
        TASK_PRIORITY_DATA,
        &data_process_task_handle,
        1  // Pin to core 1
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create data processing task");
        return ESP_FAIL;
    }
    
    // Create SD write task
    ret = xTaskCreatePinnedToCore(
        sd_write_task,
        "sd_write",
        STACK_SIZE_SD,
        NULL,
        TASK_PRIORITY_SD,
        &sd_write_task_handle,
        1  // Pin to core 1
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create SD write task");
        return ESP_FAIL;
    }
    
    // Create UI task
    ret = xTaskCreatePinnedToCore(
        ui_task,
        "ui_task",
        STACK_SIZE_UI,
        NULL,
        TASK_PRIORITY_UI,
        &ui_task_handle,
        0  // Pin to core 0 (protocol CPU)
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return ESP_FAIL;
    }
    
    // Create monitor task
    ret = xTaskCreatePinnedToCore(
        monitor_task,
        "monitor",
        STACK_SIZE_MONITOR,
        NULL,
        TASK_PRIORITY_MONITOR,
        &monitor_task_handle,
        0  // Pin to core 0
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "All tasks created successfully");
    return ESP_OK;
}

/**
 * @brief Start sensor sampling timer
 */
static esp_err_t start_sensor_timer(void)
{
    esp_timer_create_args_t timer_args = {
        .callback = &sensor_timer_callback,
        .name = "sensor_timer"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &sensor_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sensor_timer, SAMPLE_PERIOD_US));
    
    ESP_LOGI(TAG, "Sensor timer started at %d Hz", SAMPLE_RATE_HZ);
    return ESP_OK;
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Phase A Data Logger v%s", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "ESP32-S3 IoT Multi-Sensor Data Logger");
    ESP_LOGI(TAG, "===========================================");
    
    // Print system information
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Free PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    // Initialize all subsystems
    if (init_subsystems() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize subsystems");
        return;
    }
    
    // Create all tasks
    if (create_tasks() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create tasks");
        return;
    }
    
    // Start sensor sampling timer
    if (start_sensor_timer() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start sensor timer");
        return;
    }
    
    ESP_LOGI(TAG, "System initialization complete");
    ESP_LOGI(TAG, "Entering main loop...");
    
    // Main loop (this task becomes idle)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        
        // Periodic system health check
        ESP_LOGI(TAG, "System running - Free heap: %d bytes", 
                 esp_get_free_heap_size());
    }
}

// Made with Bob
