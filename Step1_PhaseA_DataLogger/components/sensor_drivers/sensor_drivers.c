/**
 * @file sensor_drivers.c
 * @brief Common sensor driver implementation
 */

#include "sensor_drivers.h"
#include "icm42688_driver.h"
#include "mmc5983_driver.h"
#include "lps22hb_driver.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "SENSOR";

// SPI mutex for thread-safe access
static SemaphoreHandle_t spi_mutex = NULL;

// Sensor handles for each module
static icm42688_handle_t icm_handles[2] = {NULL, NULL};
static mmc5983_handle_t mmc_handles[2] = {NULL, NULL};
static lps22hb_handle_t lps_handles[2] = {NULL, NULL};

// Sensor health status
static bool sensor_healthy[2] = {false, false};

// Conversion constants
#define ACCEL_SCALE_16G     (16.0f / 32768.0f)  // g per LSB
#define GYRO_SCALE_2000DPS  (2000.0f / 32768.0f) // dps per LSB
#define MAG_SCALE           (0.0625f)            // mG per LSB
#define PRESS_SCALE         (1.0f / 4096.0f)     // hPa per LSB
#define TEMP_SCALE          (1.0f / 100.0f)      // °C per LSB
#define G_TO_MS2            9.80665f             // g to m/s²
#define DPS_TO_RADS         (M_PI / 180.0f)      // dps to rad/s
#define MG_TO_UT            0.1f                 // mG to μT

/**
 * @brief Initialize all sensor drivers
 */
esp_err_t sensor_drivers_init(SemaphoreHandle_t mutex)
{
    esp_err_t ret;
    
    if (mutex == NULL) {
        ESP_LOGE(TAG, "SPI mutex is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    spi_mutex = mutex;
    ESP_LOGI(TAG, "Initializing sensor drivers...");
    
    // Initialize Module 0 sensors
    ESP_LOGI(TAG, "Initializing Module 0...");
    
    ret = icm42688_init(0, &icm_handles[0]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ICM-42688-P Module 0");
        return ret;
    }
    
    ret = mmc5983_init(0, &mmc_handles[0]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MMC5983MA Module 0");
        return ret;
    }
    
    ret = lps22hb_init(0, &lps_handles[0]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LPS22HB Module 0");
        return ret;
    }
    
    sensor_healthy[0] = true;
    ESP_LOGI(TAG, "Module 0 initialized successfully");
    
    // Initialize Module 1 sensors
    ESP_LOGI(TAG, "Initializing Module 1...");
    
    ret = icm42688_init(1, &icm_handles[1]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ICM-42688-P Module 1");
        return ret;
    }
    
    ret = mmc5983_init(1, &mmc_handles[1]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MMC5983MA Module 1");
        return ret;
    }
    
    ret = lps22hb_init(1, &lps_handles[1]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LPS22HB Module 1");
        return ret;
    }
    
    sensor_healthy[1] = true;
    ESP_LOGI(TAG, "Module 1 initialized successfully");
    
    ESP_LOGI(TAG, "All sensor drivers initialized");
    return ESP_OK;
}

/**
 * @brief Read all sensors from a specific module
 */
esp_err_t sensor_read_all_module(uint8_t module_id, sensor_sample_t *sample)
{
    if (module_id > 1 || sample == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!sensor_healthy[module_id]) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret;
    
    // Take SPI mutex
    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to take SPI mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    // Read ICM-42688-P (accel + gyro)
    ret = icm42688_read_accel_gyro(icm_handles[module_id],
                                    &sample->accel_x_raw,
                                    &sample->accel_y_raw,
                                    &sample->accel_z_raw,
                                    &sample->gyro_x_raw,
                                    &sample->gyro_y_raw,
                                    &sample->gyro_z_raw);
    if (ret != ESP_OK) {
        xSemaphoreGive(spi_mutex);
        ESP_LOGW(TAG, "Failed to read ICM-42688-P Module %d", module_id);
        return ret;
    }
    
    // Read MMC5983MA (magnetometer)
    ret = mmc5983_read_mag(mmc_handles[module_id],
                           &sample->mag_x_raw,
                           &sample->mag_y_raw,
                           &sample->mag_z_raw);
    if (ret != ESP_OK) {
        xSemaphoreGive(spi_mutex);
        ESP_LOGW(TAG, "Failed to read MMC5983MA Module %d", module_id);
        return ret;
    }
    
    // Read LPS22HB (pressure + temperature)
    ret = lps22hb_read_press_temp(lps_handles[module_id],
                                   &sample->pressure_raw,
                                   &sample->temperature_raw);
    if (ret != ESP_OK) {
        xSemaphoreGive(spi_mutex);
        ESP_LOGW(TAG, "Failed to read LPS22HB Module %d", module_id);
        return ret;
    }
    
    // Release SPI mutex
    xSemaphoreGive(spi_mutex);
    
    // Set module ID and status
    sample->module_id = module_id;
    sample->status = 0;  // TODO: Add status flags
    
    return ESP_OK;
}

/**
 * @brief Convert raw sensor data to engineering units
 */
esp_err_t sensor_convert_data(const sensor_sample_t *raw, sensor_data_t *converted)
{
    if (raw == NULL || converted == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy timestamp and module ID
    converted->timestamp_us = raw->timestamp_us;
    converted->module_id = raw->module_id;
    
    // Convert accelerometer (raw → m/s²)
    converted->accel_x_ms2 = raw->accel_x_raw * ACCEL_SCALE_16G * G_TO_MS2;
    converted->accel_y_ms2 = raw->accel_y_raw * ACCEL_SCALE_16G * G_TO_MS2;
    converted->accel_z_ms2 = raw->accel_z_raw * ACCEL_SCALE_16G * G_TO_MS2;
    
    // Convert gyroscope (raw → rad/s)
    converted->gyro_x_rads = raw->gyro_x_raw * GYRO_SCALE_2000DPS * DPS_TO_RADS;
    converted->gyro_y_rads = raw->gyro_y_raw * GYRO_SCALE_2000DPS * DPS_TO_RADS;
    converted->gyro_z_rads = raw->gyro_z_raw * GYRO_SCALE_2000DPS * DPS_TO_RADS;
    
    // Convert magnetometer (raw → μT)
    converted->mag_x_ut = raw->mag_x_raw * MAG_SCALE * MG_TO_UT;
    converted->mag_y_ut = raw->mag_y_raw * MAG_SCALE * MG_TO_UT;
    converted->mag_z_ut = raw->mag_z_raw * MAG_SCALE * MG_TO_UT;
    
    // Convert pressure (raw → hPa)
    converted->pressure_hpa = raw->pressure_raw * PRESS_SCALE;
    
    // Convert temperature (raw → °C)
    converted->temperature_c = raw->temperature_raw * TEMP_SCALE;
    
    // Set quality indicator
    converted->quality = 0;  // TODO: Implement quality assessment
    
    return ESP_OK;
}

/**
 * @brief Calibrate sensors
 */
esp_err_t sensor_calibrate(uint8_t module_id)
{
    if (module_id > 1) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Calibrating Module %d...", module_id);
    
    // Take SPI mutex
    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take SPI mutex for calibration");
        return ESP_ERR_TIMEOUT;
    }
    
    // Calibrate ICM-42688-P
    esp_err_t ret = icm42688_calibrate(icm_handles[module_id]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to calibrate ICM-42688-P Module %d", module_id);
        xSemaphoreGive(spi_mutex);
        return ret;
    }
    
    // Calibrate MMC5983MA (SET/RESET operation)
    ret = mmc5983_set_reset(mmc_handles[module_id]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to calibrate MMC5983MA Module %d", module_id);
        xSemaphoreGive(spi_mutex);
        return ret;
    }
    
    // LPS22HB doesn't require calibration
    
    xSemaphoreGive(spi_mutex);
    
    ESP_LOGI(TAG, "Module %d calibration complete", module_id);
    return ESP_OK;
}

/**
 * @brief Get sensor status
 */
bool sensor_is_healthy(uint8_t module_id)
{
    if (module_id > 1) {
        return false;
    }
    
    return sensor_healthy[module_id];
}

// Made with Bob
