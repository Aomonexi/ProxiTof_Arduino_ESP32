idf_component_register(
    SRCS "sensor_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        driver
        esp_timer
        rjrp44__vl53l8cx
        led_strip
    PRIV_REQUIRES
        esp32_BNO08x
)
