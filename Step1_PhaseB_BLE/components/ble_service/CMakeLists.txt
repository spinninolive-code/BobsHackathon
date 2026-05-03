idf_component_register(
    SRCS "ble_service.c"
    INCLUDE_DIRS "."
    REQUIRES bt nimble sensor_drivers buffer_manager sd_storage state_machine
)