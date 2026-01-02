# Time Sync Component CMakeLists.txt

idf_component_register(
    SRCS "time_sync.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        lwip
        esp_netif_stack
)