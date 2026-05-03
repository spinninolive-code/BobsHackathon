/**
 * @file buffer_manager.h
 * @brief Multi-tier buffer management (PSRAM → NAND → SD)
 */

#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sensor_drivers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer status information
 */
typedef struct {
    uint32_t psram_used;        ///< Bytes used in PSRAM
    uint32_t psram_capacity;    ///< Total PSRAM capacity
    uint32_t nand_used;         ///< Bytes used in NAND
    uint32_t nand_capacity;     ///< Total NAND capacity
    uint32_t samples_buffered;  ///< Total samples in buffers
    bool overflow_warning;      ///< True if >80% full
    uint32_t samples_written;   ///< Total samples written to SD
    uint32_t samples_lost;      ///< Total samples lost (overflow)
} buffer_status_t;

/**
 * @brief Initialize buffer manager
 * 
 * Allocates PSRAM ring buffer and initializes NAND flash cache.
 * 
 * @return ESP_OK on success
 */
esp_err_t buffer_manager_init(void);

/**
 * @brief Write sample to PSRAM buffer
 * 
 * Thread-safe function to write sensor sample to PSRAM ring buffer.
 * 
 * @param sample Pointer to sensor sample
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer full
 */
esp_err_t buffer_write_sample(const sensor_sample_t *sample);

/**
 * @brief Read samples from buffer
 * 
 * @param samples Array to store samples
 * @param count Number of samples to read
 * @return Number of samples actually read
 */
uint32_t buffer_read_samples(sensor_sample_t *samples, uint32_t count);

/**
 * @brief Get buffer status
 * 
 * @return Current buffer status
 */
buffer_status_t buffer_get_status(void);

/**
 * @brief Flush PSRAM to NAND
 * 
 * Transfers data from PSRAM ring buffer to NAND flash cache.
 * 
 * @return ESP_OK on success
 */
esp_err_t buffer_flush_psram_to_nand(void);

/**
 * @brief Flush NAND to SD
 * 
 * Transfers data from NAND flash cache to SD card.
 * This function is called by the SD storage component.
 * 
 * @return ESP_OK on success
 */
esp_err_t buffer_flush_nand_to_sd(void);

/**
 * @brief Get data from NAND for SD write
 * 
 * @param buffer Buffer to store data
 * @param size Maximum size to read
 * @return Number of bytes read
 */
uint32_t buffer_get_nand_data(uint8_t *buffer, uint32_t size);

/**
 * @brief Clear all buffers
 * 
 * @return ESP_OK on success
 */
esp_err_t buffer_clear_all(void);

/**
 * @brief Check if buffer needs flushing
 * 
 * @return true if buffer is above threshold
 */
bool buffer_needs_flush(void);

#ifdef __cplusplus
}
#endif

#endif // BUFFER_MANAGER_H

// Made with Bob
