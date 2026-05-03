/**
 * @file sd_storage.c
 * @brief SD card storage management implementation
 */

#include "sd_storage.h"
#include "buffer_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include <sys/stat.h>
#include <time.h>
#include <string.h>

static const char *TAG = "SD_STORAGE";

// Mount point
#define MOUNT_POINT "/sdcard"

// SD card handle
static sdmmc_card_t *card = NULL;
static FILE *current_file = NULL;
static sd_status_t status = {0};

// CSV header
static const char *CSV_HEADER = 
    "Timestamp_ms,Module,Sensor,Accel_X_g,Accel_Y_g,Accel_Z_g,"
    "Gyro_X_dps,Gyro_Y_dps,Gyro_Z_dps,"
    "Mag_X_uT,Mag_Y_uT,Mag_Z_uT,"
    "Pressure_hPa,Temperature_C\n";

/**
 * @brief Initialize SD card storage
 */
esp_err_t sd_storage_init(void)
{
    ESP_LOGI(TAG, "Initializing SD card storage...");
    
    // Configure SDIO host
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;  // 40MHz
    
    // Configure slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;  // 4-bit mode
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    // Mount filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config,
                                             &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // Print card info
    sdmmc_card_print_info(stdout, card);
    
    // Get card capacity
    status.total_bytes = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
    
    // Get filesystem stats
    FATFS *fs;
    DWORD fre_clust;
    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
        status.free_bytes = (uint64_t)fre_clust * fs->csize * fs->ssize;
        status.used_bytes = status.total_bytes - status.free_bytes;
    }
    
    status.mounted = true;
    status.files_created = 0;
    status.samples_written = 0;
    
    ESP_LOGI(TAG, "SD card initialized: %.2f GB total, %.2f GB free",
             status.total_bytes / (1024.0 * 1024.0 * 1024.0),
             status.free_bytes / (1024.0 * 1024.0 * 1024.0));
    
    return ESP_OK;
}

/**
 * @brief Deinitialize SD card storage
 */
esp_err_t sd_storage_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing SD card storage...");
    
    // Close current file
    if (current_file != NULL) {
        sd_storage_close_file();
    }
    
    // Unmount filesystem
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    status.mounted = false;
    card = NULL;
    
    ESP_LOGI(TAG, "SD card deinitialized");
    return ESP_OK;
}

/**
 * @brief Generate filename with timestamp
 */
static void generate_filename(char *filename, size_t size)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(filename, size, MOUNT_POINT "/data_%04d%02d%02d_%02d%02d%02d.csv",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

/**
 * @brief Start new CSV file
 */
esp_err_t sd_storage_start_file(void)
{
    if (!status.mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Close existing file
    if (current_file != NULL) {
        sd_storage_close_file();
    }
    
    // Generate filename
    char filename[64];
    generate_filename(filename, sizeof(filename));
    
    // Open file
    current_file = fopen(filename, "w");
    if (current_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filename);
        return ESP_FAIL;
    }
    
    // Write CSV header
    if (fprintf(current_file, "%s", CSV_HEADER) < 0) {
        ESP_LOGE(TAG, "Failed to write CSV header");
        fclose(current_file);
        current_file = NULL;
        return ESP_FAIL;
    }
    
    // Update status
    strncpy(status.current_filename, filename, sizeof(status.current_filename) - 1);
    status.files_created++;
    
    ESP_LOGI(TAG, "Started new CSV file: %s", filename);
    return ESP_OK;
}

/**
 * @brief Close current CSV file
 */
esp_err_t sd_storage_close_file(void)
{
    if (current_file == NULL) {
        return ESP_OK;
    }
    
    // Flush and close
    fflush(current_file);
    fclose(current_file);
    current_file = NULL;
    
    ESP_LOGI(TAG, "Closed CSV file: %s", status.current_filename);
    status.current_filename[0] = '\0';
    
    return ESP_OK;
}

/**
 * @brief Write sensor samples to SD card
 */
uint32_t sd_storage_write_samples(const sensor_sample_t *samples, uint32_t count)
{
    if (current_file == NULL) {
        ESP_LOGW(TAG, "No file open for writing");
        return 0;
    }
    
    if (samples == NULL || count == 0) {
        return 0;
    }
    
    uint32_t written = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        const sensor_sample_t *s = &samples[i];
        
        // Write CSV line
        int ret = fprintf(current_file,
            "%u,%d,%s,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            s->timestamp_ms,
            s->module_id,
            s->sensor_type == SENSOR_TYPE_ACCEL ? "ACCEL" :
            s->sensor_type == SENSOR_TYPE_GYRO ? "GYRO" :
            s->sensor_type == SENSOR_TYPE_MAG ? "MAG" :
            s->sensor_type == SENSOR_TYPE_PRESSURE ? "PRESSURE" : "UNKNOWN",
            s->accel_x, s->accel_y, s->accel_z,
            s->gyro_x, s->gyro_y, s->gyro_z,
            s->mag_x, s->mag_y, s->mag_z,
            s->pressure, s->temperature
        );
        
        if (ret > 0) {
            written++;
            status.samples_written++;
        } else {
            ESP_LOGE(TAG, "Failed to write sample %d", i);
            break;
        }
    }
    
    // Periodic flush (every 100 samples)
    if (status.samples_written % 100 == 0) {
        fflush(current_file);
    }
    
    return written;
}

/**
 * @brief Write buffer data to SD card
 */
esp_err_t sd_storage_write_buffer(const uint8_t *data, uint32_t size)
{
    if (current_file == NULL) {
        ESP_LOGW(TAG, "No file open for writing");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Cast to sensor samples
    const sensor_sample_t *samples = (const sensor_sample_t *)data;
    uint32_t count = size / sizeof(sensor_sample_t);
    
    uint32_t written = sd_storage_write_samples(samples, count);
    
    return (written == count) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Get SD card status
 */
sd_status_t sd_storage_get_status(void)
{
    // Update filesystem stats
    if (status.mounted) {
        FATFS *fs;
        DWORD fre_clust;
        if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
            status.free_bytes = (uint64_t)fre_clust * fs->csize * fs->ssize;
            status.used_bytes = status.total_bytes - status.free_bytes;
        }
    }
    
    return status;
}

/**
 * @brief Check if SD card has sufficient space
 */
bool sd_storage_has_space(uint64_t required_bytes)
{
    if (!status.mounted) {
        return false;
    }
    
    sd_status_t current_status = sd_storage_get_status();
    return (current_status.free_bytes >= required_bytes);
}

/**
 * @brief Sync/flush SD card buffers
 */
esp_err_t sd_storage_sync(void)
{
    if (current_file == NULL) {
        return ESP_OK;
    }
    
    if (fflush(current_file) != 0) {
        ESP_LOGE(TAG, "Failed to flush file");
        return ESP_FAIL;
    }
    
    // Force filesystem sync
    fsync(fileno(current_file));
    
    return ESP_OK;
}

/**
 * @brief List files on SD card
 */
esp_err_t sd_storage_list_files(void)
{
    if (!status.mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Files on SD card:");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            char filepath[128];
            snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, entry->d_name);
            
            struct stat st;
            if (stat(filepath, &st) == 0) {
                ESP_LOGI(TAG, "  %s (%ld bytes)", entry->d_name, st.st_size);
            }
        }
    }
    
    closedir(dir);
    return ESP_OK;
}

// Made with Bob
