idf_component_register(SRCS "usb_msc_sdcard.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_partition fatfs esp_timer storage_manager sdmmc
                       PRIV_REQUIRES espressif__esp_tinyusb)