/**
 * Quick SD Card SPI Test
 * Use this to test if your SD card hardware works at all
 * 
 * Wiring for SPI test:
 * ESP32-S3    SD Card
 * GPIO 11  -> MOSI (DI)
 * GPIO 13  -> MISO (DO) 
 * GPIO 12  -> SCK (CLK)
 * GPIO 7   -> CS
 * 3.3V     -> VCC
 * GND      -> GND
 */

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

void test_sd_spi(void) {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    
    // SPI host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_7;
    slot_config.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    
    if (ret == ESP_OK) {
        printf("✅ SD card works in SPI mode!\n");
        printf("Card info:\n");
        sdmmc_card_print_info(stdout, card);
        esp_vfs_fat_sdcard_unmount("/sdcard", card);
    } else {
        printf("❌ SD card failed in SPI mode too: %s\n", esp_err_to_name(ret));
    }
}