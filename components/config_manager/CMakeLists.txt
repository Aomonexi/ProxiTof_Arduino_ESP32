# Configuration Manager Component CMakeLists.txt

idf_component_register(
    SRCS "config_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        fatfs
)