# Smart Time Management Component CMakeLists.txt

idf_component_register(
    SRCS "smart_time.c"
    INCLUDE_DIRS "."
    REQUIRES 
        rtc_ds3231
        time_sync
        sensor_manager
        driver
)