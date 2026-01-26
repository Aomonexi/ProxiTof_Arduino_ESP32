/**
 * time_sync.c
 * 
 * ESP-IDF System Time Management Implementation
 * Using official ESP-IDF SNTP and system time APIs
 */

#include "time_sync.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
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
static bool s_wifi_initialized = false;

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/5)", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGW(TAG, "Connect to the AP failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "✓ Time synchronized with NTP server");
    s_time_synced = true;
}

/**
 * Validate and auto-correct common timezone mistakes
 */
static const char* validate_timezone(const char* tz) {
    if (!tz || strlen(tz) == 0) {
        ESP_LOGW(TAG, "No timezone specified, using UTC");
        return "UTC0";
    }
    
    // Common mistakes - auto-correct with warning
    if (strcmp(tz, "CST+8") == 0) {
        ESP_LOGW(TAG, "⚠️  Timezone 'CST+8' is WRONG for UTC+8!");
        ESP_LOGW(TAG, "    'CST+8' means 8 hours WEST of UTC (US Central)");
        ESP_LOGW(TAG, "    Auto-correcting to 'CST-8' (8 hours EAST of UTC)");
        return "CST-8";
    }
    if (strcmp(tz, "UTC+8") == 0) {
        ESP_LOGW(TAG, "⚠️  Timezone 'UTC+8' format is incorrect!");
        ESP_LOGW(TAG, "    Use '<+08>-8' or 'CST-8' for UTC+8");
        ESP_LOGW(TAG, "    Auto-correcting to '<+08>-8'");
        return "<+08>-8";
    }
    if (strcmp(tz, "SGT+8") == 0) {
        ESP_LOGW(TAG, "⚠️  Timezone 'SGT+8' is wrong, correcting to 'SGT-8'");
        return "SGT-8";
    }
    if (strcmp(tz, "PHT+8") == 0) {
        ESP_LOGW(TAG, "⚠️  Timezone 'PHT+8' is wrong, correcting to 'PHT-8'");
        return "PHT-8";
    }
    
    // Timezone looks reasonable
    return tz;
}

esp_err_t time_sync_init(const char* wifi_ssid, const char* wifi_password, const char* timezone)
{
    ESP_LOGI(TAG, "Initializing ESP-IDF system time management...");
    
    if (!wifi_ssid || !wifi_password) {
        ESP_LOGE(TAG, "WiFi credentials required");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = ESP_OK;
    
    // Initialize NVS (required for WiFi)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface (only once)
    if (!s_wifi_initialized) {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();
        s_wifi_initialized = true;
    }

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
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", wifi_ssid);

    // Wait for connection (30 seconds timeout)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✓ Connected to WiFi");
        
        // Validate and set timezone first (before SNTP)
        const char* validated_tz = validate_timezone(timezone);
        ESP_LOGI(TAG, "Setting timezone: %s", validated_tz);
        setenv("TZ", validated_tz, 1);
        tzset();
        
        // Configure SNTP using ESP-IDF official API
        esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        sntp_config.sync_cb = time_sync_notification_cb;
        sntp_config.smooth_sync = false;  // Use immediate sync mode
        
        ESP_LOGI(TAG, "Initializing SNTP with multiple servers...");
        ret = esp_netif_sntp_init(&sntp_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SNTP: %s", esp_err_to_name(ret));
            return ret;
        }
        
        ESP_LOGI(TAG, "Waiting for NTP time synchronization...");
        
        // Wait for time sync using ESP-IDF official API
        ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(30000));
        
        if (ret == ESP_OK) {
            // Get synchronized time
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            
            ESP_LOGI(TAG, "✓ Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            s_time_synced = true;
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "❌ Time sync timeout: %s", esp_err_to_name(ret));
            return ESP_ERR_TIMEOUT;
        }
        
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "❌ Failed to connect to WiFi");
        return ESP_ERR_WIFI_CONN;
    } else {
        ESP_LOGE(TAG, "❌ WiFi connection timeout");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t time_sync_disconnect_wifi(void) {
    ESP_LOGI(TAG, "Disconnecting WiFi to save power...");
    
    // Deinitialize SNTP
    esp_netif_sntp_deinit();
    
    // Stop WiFi
    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to stop WiFi: %s", esp_err_to_name(ret));
    }
    
    // Deinitialize WiFi
    ret = esp_wifi_deinit();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to deinit WiFi: %s", esp_err_to_name(ret));
    }
    
    // Clean up event group
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    ESP_LOGI(TAG, "✓ WiFi disconnected");
    return ESP_OK;
}

bool time_sync_is_synced(void) {
    return s_time_synced;
}

esp_err_t time_sync_get_current_time(struct tm *timeinfo) {
    if (!timeinfo) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    
    // Check if time is valid (after year 2020)
    if (now < 1577836800) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

esp_err_t time_sync_set_manual_time(const char* time_str) {
    if (!time_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct tm timeinfo = {0};
    
    // Parse time string: "YYYY-MM-DD HH:MM:SS"
    int parsed = sscanf(time_str, "%d-%d-%d %d:%d:%d",
                       &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
                       &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
    
    if (parsed != 6) {
        ESP_LOGE(TAG, "Invalid time format. Use: YYYY-MM-DD HH:MM:SS");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Adjust for tm structure
    timeinfo.tm_year -= 1900;  // Years since 1900
    timeinfo.tm_mon -= 1;      // Months since January (0-11)
    
    // Convert to time_t
    time_t t = mktime(&timeinfo);
    if (t == -1) {
        ESP_LOGE(TAG, "Invalid time values");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Set system time using POSIX function
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "Failed to set system time");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ Manual time set: %s", time_str);
    s_time_synced = true;
    
    return ESP_OK;
}