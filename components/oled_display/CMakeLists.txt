idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "sensor_manager" "storage_manager" "smart_time" "gps" "config_manager" "usb_flash_partition"
)
