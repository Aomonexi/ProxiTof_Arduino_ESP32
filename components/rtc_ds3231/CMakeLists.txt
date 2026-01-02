# DS3231 RTC Component CMakeLists.txt
# Based on nopnop2002/esp-idf-ds3231

idf_component_register(
    SRCS "ds3231.c" "i2cdev.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
)