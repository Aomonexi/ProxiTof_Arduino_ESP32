idf_component_register(
    SRCS "data_processor_c_wrapper.cpp" "data_processor.cpp"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES "esp_timer" "log" "sensor_manager"
)
