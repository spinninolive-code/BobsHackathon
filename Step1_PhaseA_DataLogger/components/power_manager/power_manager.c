/**
 * @file power_manager.c
 * @brief Battery and power management implementation
 */

#include "power_manager.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "POWER";

// Battery voltage thresholds (for 220mAh LiPo)
#define BATTERY_MAX_VOLTAGE     4200    // 4.2V fully charged
#define BATTERY_MIN_VOLTAGE     3300    // 3.3V empty
#define BATTERY_LOW_VOLTAGE     3500    // 3.5V low warning
#define BATTERY_CRITICAL_VOLTAGE 3400   // 3.4V critical

// ADC configuration
#define BATTERY_ADC_UNIT        ADC_UNIT_1
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_0  // GPIO1
#define BATTERY_ADC_ATTEN       ADC_ATTEN_DB_12
#define BATTERY_ADC_BITWIDTH    ADC_BITWIDTH_12

// Voltage divider ratio (if used)
#define VOLTAGE_DIVIDER_RATIO   2.0

// ADC handles
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool adc_calibrated = false;

// Battery status
static battery_status_t battery_status = {0};
static power_state_t power_state = POWER_STATE_NORMAL;
static SemaphoreHandle_t power_mutex = NULL;

// Callback
static battery_event_callback_t event_callback = NULL;

/**
 * @brief Initialize ADC calibration
 */
static bool init_adc_calibration(void)
{
    esp_err_t ret;
    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = BATTERY_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = BATTERY_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle);
#else
    ESP_LOGW(TAG, "ADC calibration not supported");
    return false;
#endif
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration initialized");
        return true;
    } else {
        ESP_LOGW(TAG, "ADC calibration failed: %s", esp_err_to_name(ret));
        return false;
    }
}

/**
 * @brief Initialize power manager
 */
esp_err_t power_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing power manager...");
    
    // Create mutex
    power_mutex = xSemaphoreCreateMutex();
    if (power_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create power mutex");
        return ESP_FAIL;
    }
    
    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = BATTERY_ADC_BITWIDTH,
        .atten = BATTERY_ADC_ATTEN,
    };
    
    ret = adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize calibration
    adc_calibrated = init_adc_calibration();
    
    // Initialize battery status
    battery_status.voltage_mv = 0;
    battery_status.percentage = 0;
    battery_status.charging = false;
    battery_status.low_battery = false;
    battery_status.critical_battery = false;
    
    ESP_LOGI(TAG, "Power manager initialized");
    return ESP_OK;
}

/**
 * @brief Read battery voltage
 */
static float read_battery_voltage(void)
{
    int adc_raw;
    esp_err_t ret = adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &adc_raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC: %s", esp_err_to_name(ret));
        return 0;
    }
    
    int voltage_mv;
    if (adc_calibrated) {
        ret = adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert ADC to voltage: %s", esp_err_to_name(ret));
            return 0;
        }
    } else {
        // Fallback: simple linear conversion
        voltage_mv = (adc_raw * 3300) / 4095;
    }
    
    // Apply voltage divider ratio if used
    float battery_voltage = voltage_mv * VOLTAGE_DIVIDER_RATIO;
    
    return battery_voltage;
}

/**
 * @brief Calculate battery percentage
 */
static uint8_t calculate_percentage(float voltage_mv)
{
    if (voltage_mv >= BATTERY_MAX_VOLTAGE) {
        return 100;
    } else if (voltage_mv <= BATTERY_MIN_VOLTAGE) {
        return 0;
    }
    
    // Linear interpolation
    float percentage = ((voltage_mv - BATTERY_MIN_VOLTAGE) * 100.0) / 
                       (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);
    
    return (uint8_t)percentage;
}

/**
 * @brief Update power state
 */
static void update_power_state(void)
{
    power_state_t old_state = power_state;
    
    if (battery_status.charging) {
        power_state = POWER_STATE_CHARGING;
    } else if (battery_status.critical_battery) {
        power_state = POWER_STATE_CRITICAL;
    } else if (battery_status.low_battery) {
        power_state = POWER_STATE_LOW_BATTERY;
    } else {
        power_state = POWER_STATE_NORMAL;
    }
    
    // Call callback if state changed
    if (power_state != old_state && event_callback != NULL) {
        ESP_LOGI(TAG, "Power state changed: %s -> %s",
                 power_manager_get_state_name(old_state),
                 power_manager_get_state_name(power_state));
        event_callback(power_state);
    }
}

/**
 * @brief Get battery status
 */
battery_status_t power_manager_get_status(void)
{
    xSemaphoreTake(power_mutex, portMAX_DELAY);
    
    // Read voltage
    battery_status.voltage_mv = read_battery_voltage();
    
    // Calculate percentage
    battery_status.percentage = calculate_percentage(battery_status.voltage_mv);
    
    // Check thresholds
    battery_status.low_battery = (battery_status.voltage_mv < BATTERY_LOW_VOLTAGE);
    battery_status.critical_battery = (battery_status.voltage_mv < BATTERY_CRITICAL_VOLTAGE);
    
    // Check charging (voltage increasing or above max)
    // Note: This is simplified - real implementation would track voltage over time
    battery_status.charging = (battery_status.voltage_mv > BATTERY_MAX_VOLTAGE);
    
    // Update power state
    update_power_state();
    
    xSemaphoreGive(power_mutex);
    
    return battery_status;
}

/**
 * @brief Get battery voltage
 */
float power_manager_get_voltage(void)
{
    battery_status_t status = power_manager_get_status();
    return status.voltage_mv;
}

/**
 * @brief Get battery percentage
 */
uint8_t power_manager_get_percentage(void)
{
    battery_status_t status = power_manager_get_status();
    return status.percentage;
}

/**
 * @brief Check if charging
 */
bool power_manager_is_charging(void)
{
    battery_status_t status = power_manager_get_status();
    return status.charging;
}

/**
 * @brief Check if low battery
 */
bool power_manager_is_low_battery(void)
{
    battery_status_t status = power_manager_get_status();
    return status.low_battery;
}

/**
 * @brief Check if critical battery
 */
bool power_manager_is_critical_battery(void)
{
    battery_status_t status = power_manager_get_status();
    return status.critical_battery;
}

/**
 * @brief Register callback
 */
esp_err_t power_manager_register_callback(battery_event_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    event_callback = callback;
    ESP_LOGI(TAG, "Battery event callback registered");
    
    return ESP_OK;
}

/**
 * @brief Get power state
 */
power_state_t power_manager_get_state(void)
{
    power_state_t state;
    
    xSemaphoreTake(power_mutex, portMAX_DELAY);
    state = power_state;
    xSemaphoreGive(power_mutex);
    
    return state;
}

/**
 * @brief Get power state name
 */
const char* power_manager_get_state_name(power_state_t state)
{
    switch (state) {
        case POWER_STATE_NORMAL:      return "NORMAL";
        case POWER_STATE_LOW_BATTERY: return "LOW_BATTERY";
        case POWER_STATE_CRITICAL:    return "CRITICAL";
        case POWER_STATE_CHARGING:    return "CHARGING";
        default:                      return "UNKNOWN";
    }
}

// Made with Bob
