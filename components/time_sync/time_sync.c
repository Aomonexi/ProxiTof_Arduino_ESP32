/**
 * time_sync.c
 * 
 * WiFi + NTP Time Synchronization Implementation
 */

#include "time_sync.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "TIME_SYNC";

// WiFi event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_time_synced = false;

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized with NTP server");
    s_time_synced = true;
}

esp_err_t time_sync_init(const char* wifi_ssid, const char* wifi_password, const char* timezone)
{
    ESP_LOGI(TAG, "Initializing WiFi and NTP time sync...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", wifi_ssid);

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✓ Connected to WiFi");
        
        // Set timezone
        if (timezone) {
            setenv("TZ", timezone, 1);
            tzset();
            ESP_LOGI(TAG, "Timezone set to: %s", timezone);
        }
        
        // Initialize SNTP
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_setservername(1, "time.nist.gov");
        esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
        esp_sntp_init();
        
        ESP_LOGI(TAG, "Waiting for NTP time sync...");
        
        // Wait for time sync (up to 10 seconds)
        int retry = 0;
        while (!s_time_synced && retry < 100) {
            vTaskDelay(pdMS_TO_TICKS(100));
            retry++;
        }
        
        if (s_time_synced) {
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            
            ESP_LOGI(TAG, "✓ Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "❌ Time sync timeout");
            return ESP_ERR_TIMEOUT;
        }
        
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "❌ Failed to connect to WiFi");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "❌ Unexpected WiFi event");
        return ESP_ERR_INVALID_STATE;
    }
}

bool time_sync_is_valid(void)
{
    time_t now;
    time(&now);
    return (now > 1577836800); // After Jan 1, 2020
}

esp_err_t time_sync_get_string(char* buffer, size_t size)
{
    if (!buffer || size < 20) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    return ESP_OK;
}

void time_sync_disconnect_wifi(void)
{
    ESP_LOGI(TAG, "Disconnecting WiFi to save power...");
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    ESP_LOGI(TAG, "✓ WiFi disconnected");
}