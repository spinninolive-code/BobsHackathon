/**
 * @file sd_storage.h
 * @brief SD card storage management interface
 */

#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sensor_drivers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SD card status structure
 */
typedef struct {
    bool mounted;                   ///< SD card mounted flag
    uint64_t total_bytes;          ///< Total capacity in bytes
    uint64_t used_bytes;           ///< Used space in bytes
    uint64_t free_bytes;           ///< Free space in bytes
    uint32_t files_created;        ///< Number of files created
    uint32_t samples_written;      ///< Total samples written to SD
    char current_filename[64];     ///< Current CSV filename
} sd_status_t;

/**
 * @brief Initialize SD card storage
 * 
 * Initializes SDIO interface, mounts FAT32 filesystem
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_init(void);

/**
 * @brief Deinitialize SD card storage
 * 
 * Closes files, unmounts filesystem
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_deinit(void);

/**
 * @brief Start new CSV file for data logging
 * 
 * Creates new CSV file with timestamp-based name
 * Writes CSV header
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_start_file(void);

/**
 * @brief Close current CSV file
 * 
 * Flushes buffers and closes file
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_close_file(void);

/**
 * @brief Write sensor samples to SD card
 * 
 * Writes samples in CSV format to current file
 * 
 * @param samples Array of sensor samples
 * @param count Number of samples to write
 * @return Number of samples successfully written
 */
uint32_t sd_storage_write_samples(const sensor_sample_t *samples, uint32_t count);

/**
 * @brief Write buffer data to SD card
 * 
 * Writes raw buffer data to current file
 * Used for flushing NAND cache
 * 
 * @param data Buffer data
 * @param size Size in bytes
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_write_buffer(const uint8_t *data, uint32_t size);

/**
 * @brief Get SD card status
 * 
 * @return SD card status structure
 */
sd_status_t sd_storage_get_status(void);

/**
 * @brief Check if SD card has sufficient space
 * 
 * @param required_bytes Required space in bytes
 * @return true if sufficient space available
 */
bool sd_storage_has_space(uint64_t required_bytes);

/**
 * @brief Sync/flush SD card buffers
 * 
 * Forces write of cached data to SD card
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_sync(void);

/**
 * @brief List files on SD card
 * 
 * Lists all CSV files in root directory
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_storage_list_files(void);

#ifdef __cplusplus
}
#endif

#endif // SD_STORAGE_H

// Made with Bob
