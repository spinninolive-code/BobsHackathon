/**
 * @file buffer_manager.c
 * @brief Multi-tier buffer management implementation
 */

#include "buffer_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "BUFFER";

// Buffer configuration
#define PSRAM_BUFFER_SIZE       (4 * 1024 * 1024)  // 4MB
#define NAND_CACHE_SIZE         (8 * 1024 * 1024)  // 8MB
#define SAMPLE_SIZE             sizeof(sensor_sample_t)
#define MAX_SAMPLES             (PSRAM_BUFFER_SIZE / SAMPLE_SIZE)

// PSRAM ring buffer
static sensor_sample_t *psram_buffer = NULL;
static uint32_t psram_write_idx = 0;
static uint32_t psram_read_idx = 0;
static uint32_t psram_count = 0;
static SemaphoreHandle_t psram_mutex = NULL;

// NAND flash cache
static const esp_partition_t *nand_partition = NULL;
static uint32_t nand_write_offset = 0;
static uint32_t nand_read_offset = 0;
static uint32_t nand_used_bytes = 0;
static SemaphoreHandle_t nand_mutex = NULL;

// Statistics
static uint32_t total_samples_written = 0;
static uint32_t total_samples_lost = 0;

/**
 * @brief Initialize buffer manager
 */
esp_err_t buffer_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing buffer manager...");
    
    // Create mutexes
    psram_mutex = xSemaphoreCreateMutex();
    if (psram_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create PSRAM mutex");
        return ESP_FAIL;
    }
    
    nand_mutex = xSemaphoreCreateMutex();
    if (nand_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create NAND mutex");
        return ESP_FAIL;
    }
    
    // Allocate PSRAM buffer
    psram_buffer = (sensor_sample_t *)heap_caps_malloc(PSRAM_BUFFER_SIZE, 
                                                        MALLOC_CAP_SPIRAM);
    if (psram_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffer");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "PSRAM buffer allocated: %d bytes (%d samples)",
             PSRAM_BUFFER_SIZE, MAX_SAMPLES);
    
    // Find NAND flash partition
    nand_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                               ESP_PARTITION_SUBTYPE_DATA_FAT,
                                               "data_cache");
    if (nand_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find NAND cache partition");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "NAND cache partition found: %d bytes", nand_partition->size);
    
    // Erase NAND partition
    ESP_LOGI(TAG, "Erasing NAND cache partition...");
    esp_err_t ret = esp_partition_erase_range(nand_partition, 0, nand_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase NAND partition: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize indices
    psram_write_idx = 0;
    psram_read_idx = 0;
    psram_count = 0;
    nand_write_offset = 0;
    nand_read_offset = 0;
    nand_used_bytes = 0;
    
    ESP_LOGI(TAG, "Buffer manager initialized successfully");
    return ESP_OK;
}

/**
 * @brief Write sample to PSRAM buffer
 */
esp_err_t buffer_write_sample(const sensor_sample_t *sample)
{
    if (sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Take mutex
    if (xSemaphoreTake(psram_mutex, pdMS_TO_TICKS(1)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Check if buffer is full
    if (psram_count >= MAX_SAMPLES) {
        xSemaphoreGive(psram_mutex);
        total_samples_lost++;
        ESP_LOGW(TAG, "PSRAM buffer full! Sample lost (total lost: %d)", 
                 total_samples_lost);
        return ESP_ERR_NO_MEM;
    }
    
    // Write sample
    memcpy(&psram_buffer[psram_write_idx], sample, SAMPLE_SIZE);
    
    // Update indices
    psram_write_idx = (psram_write_idx + 1) % MAX_SAMPLES;
    psram_count++;
    total_samples_written++;
    
    xSemaphoreGive(psram_mutex);
    return ESP_OK;
}

/**
 * @brief Read samples from buffer
 */
uint32_t buffer_read_samples(sensor_sample_t *samples, uint32_t count)
{
    if (samples == NULL || count == 0) {
        return 0;
    }
    
    // Take mutex
    if (xSemaphoreTake(psram_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return 0;
    }
    
    // Limit to available samples
    uint32_t to_read = (count < psram_count) ? count : psram_count;
    
    // Read samples
    for (uint32_t i = 0; i < to_read; i++) {
        memcpy(&samples[i], &psram_buffer[psram_read_idx], SAMPLE_SIZE);
        psram_read_idx = (psram_read_idx + 1) % MAX_SAMPLES;
        psram_count--;
    }
    
    xSemaphoreGive(psram_mutex);
    return to_read;
}

/**
 * @brief Get buffer status
 */
buffer_status_t buffer_get_status(void)
{
    buffer_status_t status = {0};
    
    // Take mutexes
    xSemaphoreTake(psram_mutex, portMAX_DELAY);
    xSemaphoreTake(nand_mutex, portMAX_DELAY);
    
    status.psram_used = psram_count * SAMPLE_SIZE;
    status.psram_capacity = PSRAM_BUFFER_SIZE;
    status.nand_used = nand_used_bytes;
    status.nand_capacity = NAND_CACHE_SIZE;
    status.samples_buffered = psram_count + (nand_used_bytes / SAMPLE_SIZE);
    status.samples_written = total_samples_written;
    status.samples_lost = total_samples_lost;
    
    // Check for overflow warning (>80% full)
    uint32_t psram_percent = (status.psram_used * 100) / status.psram_capacity;
    uint32_t nand_percent = (status.nand_used * 100) / status.nand_capacity;
    status.overflow_warning = (psram_percent > 80) || (nand_percent > 80);
    
    xSemaphoreGive(nand_mutex);
    xSemaphoreGive(psram_mutex);
    
    return status;
}

/**
 * @brief Flush PSRAM to NAND
 */
esp_err_t buffer_flush_psram_to_nand(void)
{
    // Check if NAND has space
    if (nand_used_bytes >= NAND_CACHE_SIZE) {
        ESP_LOGW(TAG, "NAND cache full, cannot flush PSRAM");
        return ESP_ERR_NO_MEM;
    }
    
    // Calculate how many samples to transfer
    uint32_t samples_to_transfer = psram_count / 2;  // Transfer half
    if (samples_to_transfer == 0) {
        return ESP_OK;  // Nothing to transfer
    }
    
    // Limit by NAND space
    uint32_t nand_space = NAND_CACHE_SIZE - nand_used_bytes;
    uint32_t max_samples = nand_space / SAMPLE_SIZE;
    if (samples_to_transfer > max_samples) {
        samples_to_transfer = max_samples;
    }
    
    ESP_LOGI(TAG, "Flushing %d samples from PSRAM to NAND", samples_to_transfer);
    
    // Allocate temporary buffer
    sensor_sample_t *temp_buffer = malloc(samples_to_transfer * SAMPLE_SIZE);
    if (temp_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate temp buffer for NAND flush");
        return ESP_ERR_NO_MEM;
    }
    
    // Read from PSRAM
    uint32_t samples_read = buffer_read_samples(temp_buffer, samples_to_transfer);
    
    // Write to NAND
    xSemaphoreTake(nand_mutex, portMAX_DELAY);
    
    esp_err_t ret = esp_partition_write(nand_partition, nand_write_offset,
                                        temp_buffer, samples_read * SAMPLE_SIZE);
    if (ret == ESP_OK) {
        nand_write_offset += samples_read * SAMPLE_SIZE;
        nand_used_bytes += samples_read * SAMPLE_SIZE;
        ESP_LOGI(TAG, "Wrote %d samples to NAND (offset: %d, used: %d bytes)",
                 samples_read, nand_write_offset, nand_used_bytes);
    } else {
        ESP_LOGE(TAG, "Failed to write to NAND: %s", esp_err_to_name(ret));
    }
    
    xSemaphoreGive(nand_mutex);
    free(temp_buffer);
    
    return ret;
}

/**
 * @brief Flush NAND to SD
 */
esp_err_t buffer_flush_nand_to_sd(void)
{
    ESP_LOGI(TAG, "Flushing NAND to SD (stub - handled by SD storage component)");
    
    // This is called by the SD storage component
    // The actual data transfer is done via buffer_get_nand_data()
    
    return ESP_OK;
}

/**
 * @brief Get data from NAND for SD write
 */
uint32_t buffer_get_nand_data(uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || size == 0) {
        return 0;
    }
    
    xSemaphoreTake(nand_mutex, portMAX_DELAY);
    
    // Limit to available data
    uint32_t to_read = (size < nand_used_bytes) ? size : nand_used_bytes;
    
    if (to_read > 0) {
        // Read from NAND
        esp_err_t ret = esp_partition_read(nand_partition, nand_read_offset,
                                           buffer, to_read);
        if (ret == ESP_OK) {
            nand_read_offset += to_read;
            nand_used_bytes -= to_read;
            
            // Reset offsets if empty
            if (nand_used_bytes == 0) {
                nand_write_offset = 0;
                nand_read_offset = 0;
            }
        } else {
            ESP_LOGE(TAG, "Failed to read from NAND: %s", esp_err_to_name(ret));
            to_read = 0;
        }
    }
    
    xSemaphoreGive(nand_mutex);
    return to_read;
}

/**
 * @brief Clear all buffers
 */
esp_err_t buffer_clear_all(void)
{
    ESP_LOGI(TAG, "Clearing all buffers...");
    
    xSemaphoreTake(psram_mutex, portMAX_DELAY);
    xSemaphoreTake(nand_mutex, portMAX_DELAY);
    
    // Reset PSRAM
    psram_write_idx = 0;
    psram_read_idx = 0;
    psram_count = 0;
    
    // Reset NAND
    nand_write_offset = 0;
    nand_read_offset = 0;
    nand_used_bytes = 0;
    
    xSemaphoreGive(nand_mutex);
    xSemaphoreGive(psram_mutex);
    
    ESP_LOGI(TAG, "All buffers cleared");
    return ESP_OK;
}

/**
 * @brief Check if buffer needs flushing
 */
bool buffer_needs_flush(void)
{
    buffer_status_t status = buffer_get_status();
    
    // Flush if PSRAM >50% or NAND >75%
    uint32_t psram_percent = (status.psram_used * 100) / status.psram_capacity;
    uint32_t nand_percent = (status.nand_used * 100) / status.nand_capacity;
    
    return (psram_percent > 50) || (nand_percent > 75);
}

// Made with Bob
