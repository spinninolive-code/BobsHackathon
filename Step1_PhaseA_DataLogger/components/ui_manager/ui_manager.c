/**
 * @file ui_manager.c
 * @brief User interface management implementation
 */

#include "ui_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "UI_MANAGER";

// DIP switch GPIO pins (from app_config.h)
#define DIP_SW1_PIN     GPIO_NUM_4
#define DIP_SW2_PIN     GPIO_NUM_5
#define DIP_SW3_PIN     GPIO_NUM_6

// LED GPIO pin
#define LED_PIN         GPIO_NUM_48

// LED strip handle
static led_strip_handle_t led_strip = NULL;

// Current mode
static dip_mode_t current_mode = MODE_IDLE;
static SemaphoreHandle_t ui_mutex = NULL;

// Mode change callback
static mode_change_callback_t mode_callback = NULL;

// LED pattern task
static TaskHandle_t led_task_handle = NULL;
static led_color_t current_led_color = LED_OFF;
static led_pattern_t current_led_pattern = LED_PATTERN_SOLID;

/**
 * @brief RGB color values for each LED color
 */
static const struct {
    uint8_t r, g, b;
} color_map[] = {
    [LED_OFF]     = {0,   0,   0},
    [LED_RED]     = {255, 0,   0},
    [LED_GREEN]   = {0,   255, 0},
    [LED_BLUE]    = {0,   0,   255},
    [LED_YELLOW]  = {255, 255, 0},
    [LED_CYAN]    = {0,   255, 255},
    [LED_MAGENTA] = {255, 0,   255},
    [LED_WHITE]   = {255, 255, 255}
};

/**
 * @brief LED pattern task
 */
static void led_pattern_task(void *arg)
{
    uint32_t counter = 0;
    
    while (1) {
        uint8_t r = color_map[current_led_color].r;
        uint8_t g = color_map[current_led_color].g;
        uint8_t b = color_map[current_led_color].b;
        
        switch (current_led_pattern) {
            case LED_PATTERN_SOLID:
                // Solid color
                led_strip_set_pixel(led_strip, 0, r, g, b);
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
                
            case LED_PATTERN_BLINK_SLOW:
                // 1Hz blink
                if (counter % 10 < 5) {
                    led_strip_set_pixel(led_strip, 0, r, g, b);
                } else {
                    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                }
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(100));
                counter++;
                break;
                
            case LED_PATTERN_BLINK_FAST:
                // 4Hz blink
                if (counter % 2 == 0) {
                    led_strip_set_pixel(led_strip, 0, r, g, b);
                } else {
                    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
                }
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(125));
                counter++;
                break;
                
            case LED_PATTERN_PULSE:
                // Breathing effect
                for (int i = 0; i < 100; i += 5) {
                    uint8_t brightness = (i < 50) ? (i * 2) : ((100 - i) * 2);
                    led_strip_set_pixel(led_strip, 0,
                                       (r * brightness) / 100,
                                       (g * brightness) / 100,
                                       (b * brightness) / 100);
                    led_strip_refresh(led_strip);
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                break;
        }
    }
}

/**
 * @brief Initialize UI manager
 */
esp_err_t ui_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing UI manager...");
    
    // Create mutex
    ui_mutex = xSemaphoreCreateMutex();
    if (ui_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create UI mutex");
        return ESP_FAIL;
    }
    
    // Configure DIP switch GPIOs as inputs with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DIP_SW1_PIN) | (1ULL << DIP_SW2_PIN) | (1ULL << DIP_SW3_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DIP switch GPIOs: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure WS2812 LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    
    ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Turn LED off initially
    led_strip_clear(led_strip);
    
    // Create LED pattern task
    xTaskCreate(led_pattern_task, "led_pattern", 2048, NULL, 5, &led_task_handle);
    
    // Read initial mode
    current_mode = ui_manager_read_mode();
    
    ESP_LOGI(TAG, "UI manager initialized (mode: %s)",
             ui_manager_get_mode_name(current_mode));
    
    return ESP_OK;
}

/**
 * @brief Read DIP switch mode
 */
dip_mode_t ui_manager_read_mode(void)
{
    // Read DIP switches (active low with pull-up)
    uint8_t sw1 = !gpio_get_level(DIP_SW1_PIN);
    uint8_t sw2 = !gpio_get_level(DIP_SW2_PIN);
    uint8_t sw3 = !gpio_get_level(DIP_SW3_PIN);
    
    // Combine into mode value
    dip_mode_t new_mode = (dip_mode_t)((sw3 << 2) | (sw2 << 1) | sw1);
    
    xSemaphoreTake(ui_mutex, portMAX_DELAY);
    
    // Check if mode changed
    if (new_mode != current_mode) {
        dip_mode_t old_mode = current_mode;
        current_mode = new_mode;
        
        ESP_LOGI(TAG, "Mode changed: %s -> %s",
                 ui_manager_get_mode_name(old_mode),
                 ui_manager_get_mode_name(new_mode));
        
        // Call callback if registered
        if (mode_callback != NULL) {
            mode_callback(old_mode, new_mode);
        }
    }
    
    xSemaphoreGive(ui_mutex);
    
    return current_mode;
}

/**
 * @brief Set LED color
 */
esp_err_t ui_manager_set_led_color(led_color_t color)
{
    return ui_manager_set_led_pattern(color, LED_PATTERN_SOLID);
}

/**
 * @brief Set LED pattern
 */
esp_err_t ui_manager_set_led_pattern(led_color_t color, led_pattern_t pattern)
{
    if (color >= sizeof(color_map) / sizeof(color_map[0])) {
        return ESP_ERR_INVALID_ARG;
    }
    
    xSemaphoreTake(ui_mutex, portMAX_DELAY);
    current_led_color = color;
    current_led_pattern = pattern;
    xSemaphoreGive(ui_mutex);
    
    return ESP_OK;
}

/**
 * @brief Set LED RGB values
 */
esp_err_t ui_manager_set_led_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    return led_strip_refresh(led_strip);
}

/**
 * @brief Turn LED off
 */
esp_err_t ui_manager_led_off(void)
{
    return ui_manager_set_led_color(LED_OFF);
}

/**
 * @brief Register mode change callback
 */
esp_err_t ui_manager_register_callback(mode_change_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    mode_callback = callback;
    ESP_LOGI(TAG, "Mode change callback registered");
    
    return ESP_OK;
}

/**
 * @brief Get mode name
 */
const char* ui_manager_get_mode_name(dip_mode_t mode)
{
    switch (mode) {
        case MODE_IDLE:          return "IDLE";
        case MODE_ACQUIRE:       return "ACQUIRE";
        case MODE_STORE:         return "STORE";
        case MODE_ACQUIRE_STORE: return "ACQUIRE_STORE";
        case MODE_TEST:          return "TEST";
        case MODE_RESERVED_5:    return "RESERVED_5";
        case MODE_RESERVED_6:    return "RESERVED_6";
        case MODE_SHUTDOWN:      return "SHUTDOWN";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief Update LED for state
 */
esp_err_t ui_manager_update_led_for_state(const char* state_name)
{
    if (state_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Map state names to LED colors/patterns
    if (strcmp(state_name, "INIT") == 0) {
        return ui_manager_set_led_pattern(LED_WHITE, LED_PATTERN_PULSE);
    } else if (strcmp(state_name, "IDLE") == 0) {
        return ui_manager_set_led_pattern(LED_GREEN, LED_PATTERN_SOLID);
    } else if (strcmp(state_name, "ACQUIRING") == 0) {
        return ui_manager_set_led_pattern(LED_BLUE, LED_PATTERN_BLINK_SLOW);
    } else if (strcmp(state_name, "STORING") == 0) {
        return ui_manager_set_led_pattern(LED_CYAN, LED_PATTERN_BLINK_FAST);
    } else if (strcmp(state_name, "ERROR") == 0) {
        return ui_manager_set_led_pattern(LED_RED, LED_PATTERN_BLINK_FAST);
    } else if (strcmp(state_name, "LOW_BATTERY") == 0) {
        return ui_manager_set_led_pattern(LED_YELLOW, LED_PATTERN_BLINK_SLOW);
    } else if (strcmp(state_name, "SHUTDOWN") == 0) {
        return ui_manager_set_led_pattern(LED_RED, LED_PATTERN_SOLID);
    }
    
    return ESP_OK;
}

// Made with Bob
