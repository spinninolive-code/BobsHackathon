idf_component_register(
    SRCS "buffer_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        esp_timer
        spi_flash
        sensor_drivers
)