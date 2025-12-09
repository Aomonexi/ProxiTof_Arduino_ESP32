idf_component_register(
    SRCS "data_processor.cpp"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES "esp_timer" "log"
)
