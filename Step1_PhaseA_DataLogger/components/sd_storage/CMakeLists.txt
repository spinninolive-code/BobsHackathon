idf_component_register(
    SRCS "sd_storage.c"
    INCLUDE_DIRS "."
    REQUIRES driver fatfs sdmmc esp_timer buffer_manager
)