# USB Interface Component CMakeLists.txt

idf_component_register(
    SRCS "usb_interface.c"
    INCLUDE_DIRS "."
    REQUIRES 
        smart_time
        config_manager
        driver
)