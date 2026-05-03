idf_component_register(
    SRCS 
        "sensor_drivers.c"
        "icm42688_driver.c"
        "mmc5983_driver.c"
        "lps22hb_driver.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        esp_timer
)