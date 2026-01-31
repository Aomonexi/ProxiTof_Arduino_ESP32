idf_component_register(
    SRCS "storage_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES fatfs sdmmc driver esp_timer smart_time config_manager
)
