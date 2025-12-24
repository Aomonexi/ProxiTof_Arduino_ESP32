# DS3231 RTC Component CMakeLists.txt

idf_component_register(
    SRCS "rtc_ds3231.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        esp_driver_i2c
)