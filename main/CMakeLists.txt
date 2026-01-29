# Main component CMakeLists.txt

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        sensor_manager
        storage_manager
        data_processor
        smart_time
        config_manager
        usb_interface
        usb_flash_partition
        display_manager
        oled_display
        gps
        buzzer
        led_strip
        driver
)
