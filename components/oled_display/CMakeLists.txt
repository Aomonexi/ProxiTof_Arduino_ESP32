idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_timer" "k0i05__esp_ssd1306" "sensor_manager" "storage_manager"
)
