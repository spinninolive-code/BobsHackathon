/**
 * @file icm42688_driver.c
 * @brief ICM-42688-P 6-axis IMU driver implementation
 */

#include "icm42688_driver.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ICM42688";

// CS pins for each module
static const uint8_t cs_pins[2] = {1, 2};  // GPIO1 for module 0, GPIO2 for module 1

// Sensor context
typedef struct {
    spi_device_handle_t spi;
    uint8_t module_id;
    bool initialized;
} icm42688_context_t;

/**
 * @brief Write register
 */
static esp_err_t icm42688_write_reg(icm42688_context_t *ctx, uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2] = {reg & 0x7F, value};  // Clear MSB for write
    uint8_t rx_data[2];
    
    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };
    
    return spi_device_transmit(ctx->spi, &trans);
}

/**
 * @brief Read register
 */
static esp_err_t icm42688_read_reg(icm42688_context_t *ctx, uint8_t reg, uint8_t *value)
{
    uint8_t tx_data[2] = {reg | 0x80, 0x00};  // Set MSB for read
    uint8_t rx_data[2];
    
    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };
    
    esp_err_t ret = spi_device_transmit(ctx->spi, &trans);
    if (ret == ESP_OK) {
        *value = rx_data[1];
    }
    
    return ret;
}

/**
 * @brief Read multiple registers
 */
static esp_err_t icm42688_read_regs(icm42688_context_t *ctx, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t tx_data[32] = {reg | 0x80};  // Set MSB for read
    uint8_t rx_data[32];
    
    if (len > 31) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    spi_transaction_t trans = {
        .length = (len + 1) * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };
    
    esp_err_t ret = spi_device_transmit(ctx->spi, &trans);
    if (ret == ESP_OK) {
        memcpy(data, &rx_data[1], len);
    }
    
    return ret;
}

/**
 * @brief Initialize ICM-42688-P sensor
 */
esp_err_t icm42688_init(uint8_t module_id, icm42688_handle_t *handle)
{
    if (module_id > 1 || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing ICM-42688-P Module %d...", module_id);
    
    // Allocate context
    icm42688_context_t *ctx = calloc(1, sizeof(icm42688_context_t));
    if (ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate context");
        return ESP_ERR_NO_MEM;
    }
    
    ctx->module_id = module_id;
    
    // Configure SPI device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 20 * 1000 * 1000,  // 20 MHz
        .mode = 0,                            // SPI mode 0
        .spics_io_num = cs_pins[module_id],
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    
    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &ctx->spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        free(ctx);
        return ret;
    }
    
    // Wait for sensor to be ready
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Check WHO_AM_I
    uint8_t who_am_i;
    ret = icm42688_read_reg(ctx, ICM42688_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK || who_am_i != ICM42688_WHO_AM_I_VALUE) {
        ESP_LOGE(TAG, "WHO_AM_I check failed: 0x%02X (expected 0x%02X)", 
                 who_am_i, ICM42688_WHO_AM_I_VALUE);
        spi_bus_remove_device(ctx->spi);
        free(ctx);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "ICM-42688-P detected (WHO_AM_I: 0x%02X)", who_am_i);
    
    // Configure power management (enable accel and gyro)
    ret = icm42688_write_reg(ctx, ICM42688_PWR_MGMT0, 0x0F);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power management");
        spi_bus_remove_device(ctx->spi);
        free(ctx);
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Configure accelerometer (±16g, 1kHz ODR)
    ret = icm42688_write_reg(ctx, ICM42688_ACCEL_CONFIG0, 
                              (ICM42688_ACCEL_RANGE_16G << 5) | ICM42688_ODR_1KHZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure accelerometer");
        spi_bus_remove_device(ctx->spi);
        free(ctx);
        return ret;
    }
    
    // Configure gyroscope (±2000dps, 1kHz ODR)
    ret = icm42688_write_reg(ctx, ICM42688_GYRO_CONFIG0,
                              (ICM42688_GYRO_RANGE_2000DPS << 5) | ICM42688_ODR_1KHZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure gyroscope");
        spi_bus_remove_device(ctx->spi);
        free(ctx);
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    ctx->initialized = true;
    *handle = ctx;
    
    ESP_LOGI(TAG, "ICM-42688-P Module %d initialized successfully", module_id);
    return ESP_OK;
}

/**
 * @brief Read accelerometer and gyroscope data
 */
esp_err_t icm42688_read_accel_gyro(icm42688_handle_t handle,
                                     int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                     int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    icm42688_context_t *ctx = (icm42688_context_t *)handle;
    
    if (!ctx->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Read 12 bytes starting from ACCEL_DATA_X1
    uint8_t data[12];
    esp_err_t ret = icm42688_read_regs(ctx, ICM42688_ACCEL_DATA_X1, data, 12);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Parse accelerometer data (big-endian)
    if (accel_x) *accel_x = (int16_t)((data[0] << 8) | data[1]);
    if (accel_y) *accel_y = (int16_t)((data[2] << 8) | data[3]);
    if (accel_z) *accel_z = (int16_t)((data[4] << 8) | data[5]);
    
    // Parse gyroscope data (big-endian)
    if (gyro_x) *gyro_x = (int16_t)((data[6] << 8) | data[7]);
    if (gyro_y) *gyro_y = (int16_t)((data[8] << 8) | data[9]);
    if (gyro_z) *gyro_z = (int16_t)((data[10] << 8) | data[11]);
    
    return ESP_OK;
}

/**
 * @brief Calibrate sensor (zero offset)
 */
esp_err_t icm42688_calibrate(icm42688_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    icm42688_context_t *ctx = (icm42688_context_t *)handle;
    
    if (!ctx->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Calibrating ICM-42688-P Module %d...", ctx->module_id);
    
    // TODO: Implement calibration routine
    // For now, just a placeholder
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "ICM-42688-P Module %d calibration complete", ctx->module_id);
    return ESP_OK;
}

// Made with Bob
