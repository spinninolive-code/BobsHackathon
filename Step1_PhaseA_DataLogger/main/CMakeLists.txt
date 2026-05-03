idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        sensor_drivers
        buffer_manager
        sd_storage
        state_machine
        power_manager
        ui_manager
        driver
        esp_timer
        nvs_flash
        fatfs
        sdmmc
)