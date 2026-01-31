# Main component CMakeLists.txt

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "." "../components/ml_inference/include"
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
        ml_inference
)
