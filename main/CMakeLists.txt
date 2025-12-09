# Main component CMakeLists.txt

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        sensor_manager
        led_strip
        driver
)
