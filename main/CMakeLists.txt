# Main component CMakeLists.txt

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        sensor_manager
        storage_manager
        data_processor
        led_strip
        driver
)
