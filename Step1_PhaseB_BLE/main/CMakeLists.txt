idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos 
        driver 
        nvs_flash
        bt
        nimble
        sensor_drivers 
        buffer_manager 
        sd_storage 
        state_machine 
        power_manager 
        ui_manager
        ble_service
)