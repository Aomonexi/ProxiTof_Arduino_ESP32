idf_component_register(SRCS "display_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES "esp_timer" "freertos" "log" "sensor_manager" "storage_manager")