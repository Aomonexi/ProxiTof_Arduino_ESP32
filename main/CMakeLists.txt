# Main component CMakeLists.txt
# Using proxitof_ag_main.cpp (working implementation) for testing

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        rjrp44__vl53l8cx
        esp_timer
        driver
        led_strip
)
