# USB Mass Storage Component CMakeLists.txt

idf_component_register(
    SRCS "usb_msc.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        fatfs
        storage_manager
        espressif__esp_tinyusb
        esp_driver_sdmmc
        sdmmc
        freertos
)