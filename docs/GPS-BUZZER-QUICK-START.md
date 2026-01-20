# GPS and Buzzer Quick Start Guide

**5-Minute Overview** | **Status**: Ready for Integration

---

## 🔌 Hardware Connections

```
Neo-M9N GPS          ESP32-S3
-----------          --------
VCC (5V)       →     5V
GND            →     GND
TX             →     GPIO 43 (RX)  ← CROSSOVER!
RX             ←     GPIO 44 (TX)  ← CROSSOVER!

Buzzer               ESP32-S3
------               --------
IN             ←     GPIO 45 (PWM)
VCC            →     5V
GND            →     GND

Potentiometer        ESP32-S3 (OPTIONAL)
-------------        --------
Wiper          →     GPIO 46 (ADC)  ✅ NOT GPIO 4!
One end        →     3.3V
Other end      →     GND
```

**⚠️ IMPORTANT NOTES:**
1. **UART is crossover**: ESP32 TX→GPS RX, ESP32 RX←GPS TX (this is correct!)
2. **GPIO 4 is occupied**: Used by ToF sensor, use GPIO 46 for potentiometer
3. **GPS needs 5V**: Not 3.3V!

---

## 📝 Code Integration (Copy-Paste Ready)

### 1. Update `main/CMakeLists.txt`

Add `gps` and `buzzer` to REQUIRES:
```cmake
REQUIRES 
    sensor_manager
    storage_manager
    data_processor
    smart_time
    config_manager
    usb_interface
    usb_flash_partition
    display_manager
    oled_display
    gps              # ADD
    buzzer           # ADD
    led_strip
    driver
```

### 2. Add Includes to `main/main.c`

After existing includes:
```c
#include "gps.h"
#include "buzzer.h"
```

### 3. Add Pin Definitions

After LED configuration:
```c
// GPS Configuration
#define GPS_UART_NUM UART_NUM_1
#define GPS_TX_PIN GPIO_NUM_44
#define GPS_RX_PIN GPIO_NUM_43
#define GPS_BAUD_RATE 9600

// Buzzer Configuration
#define BUZZER_PIN GPIO_NUM_45
#define BUZZER_DEFAULT_FREQ 2000
#define BUZZER_DEFAULT_VOLUME 50
```

### 4. Initialize in `app_main()`

After data processor initialization (around line 360):
```c
// =========================================================================
// GPS MODULE INITIALIZATION
// =========================================================================
ESP_LOGI(TAG, "Initializing GPS module...");
err = gps_init(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
if (err == ESP_OK) {
    ESP_LOGI(TAG, "✓ GPS initialized");
} else {
    ESP_LOGW(TAG, "GPS init failed: %s", esp_err_to_name(err));
}

// =========================================================================
// BUZZER INITIALIZATION
// =========================================================================
ESP_LOGI(TAG, "Initializing buzzer...");
buzzer_config_t buzzer_config = {
    .gpio_pin = BUZZER_PIN,
    .adc_pin = -1,
    .default_frequency_hz = BUZZER_DEFAULT_FREQ,
    .default_volume_percent = BUZZER_DEFAULT_VOLUME,
    .use_potentiometer = false
};
err = buzzer_init(&buzzer_config);
if (err == ESP_OK) {
    ESP_LOGI(TAG, "✓ Buzzer initialized");
    buzzer_play_pattern(BUZZER_PATTERN_STARTUP);
    vTaskDelay(pdMS_TO_TICKS(500));
} else {
    ESP_LOGW(TAG, "Buzzer init failed: %s", esp_err_to_name(err));
}
ESP_LOGI(TAG, "");
```

### 5. Read GPS in Main Loop

Add after sensor reading (around line 550):
```c
// Read GPS data (non-blocking)
gps_data_t gps_data = {0};
esp_err_t gps_err = gps_read(&gps_data);

// Debug GPS every 5 seconds
static uint32_t last_gps_debug = 0;
if ((current_time - last_gps_debug) >= 5000) {
    last_gps_debug = current_time;
    if (gps_err == ESP_OK) {
        ESP_LOGI(TAG, "📍 GPS: %.6f, %.6f | Speed: %.1f m/s | Sats: %d",
                 gps_data.latitude, gps_data.longitude,
                 gps_data.speed_ms, gps_data.satellites_used);
    }
}
```

### 6. Add GPS to Logging

Update logging section (around line 650):
```c
// Copy GPS data
if (gps_err == ESP_OK) {
    log_entry.latitude = gps_data.latitude;
    log_entry.longitude = gps_data.longitude;
    log_entry.speed_ms = gps_data.speed_ms;
    log_entry.heading_deg = gps_data.heading_deg;
    log_entry.gps_satellites = gps_data.satellites_used;
} else {
    log_entry.latitude = 0.0;
    log_entry.longitude = 0.0;
    log_entry.speed_ms = 0.0;
    log_entry.heading_deg = 0.0;
    log_entry.gps_satellites = 0;
}

// Set automatic flag
log_entry.flag_auto = near_miss_active ? 1 : 0;
```

### 7. Trigger Buzzer on Near-Miss

In near-miss detection (around line 590):
```c
if (near_miss_detected) {
    if (!near_miss_active) {
        // New near-miss event
        near_miss_active = true;
        near_miss_start_time = current_time;
        near_miss_event_count++;
        
        // Trigger buzzer alert
        buzzer_play_pattern(BUZZER_PATTERN_NEAR_MISS);
        
        // ... existing logging code ...
    }
}
```

### 8. Trigger Buzzer on Button

In button handling (around line 680):
```c
if (sensor_manager_is_button_pressed()) {
    ESP_LOGI(TAG, "✓ EVENT FLAGGED");
    buzzer_play_pattern(BUZZER_PATTERN_BUTTON);
}
```

---

## 🔨 Build and Flash

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Build
idf.py build

# Flash and monitor
idf.py -p COM3 flash monitor
```

---

## ✅ Expected Output

```
GPS: Initializing...
✓ GPS initialized
Buzzer: Initializing...
✓ Buzzer initialized
[Startup melody plays]

... system starts ...

GPS: Waiting for fix...
GPS: Waiting for fix...
GPS: ✓ First fix acquired in 28 seconds
📍 GPS: 37.123456, -122.987654 | Speed: 0.5 m/s | Sats: 8

[When near-miss detected]
⚠️  NEAR-MISS EVENT DETECTED #1
[3 short beeps play]

[When button pressed]
✓ EVENT FLAGGED
[Single beep plays]
```

---

## 🐛 Troubleshooting

### GPS Not Working
- Check wiring (TX=44, RX=43, 5V, GND)
- Wait 26 seconds for cold start
- Move to open sky (GPS doesn't work indoors)
- Check serial output for errors

### Buzzer Not Working
- Check wiring (IN=45, 5V, GND)
- Verify buzzer type (active vs passive)
- Check if startup melody plays
- Adjust volume: `buzzer_set_volume(100)`

### Build Errors
- Make sure GPS and buzzer are in CMakeLists.txt
- Check includes are added
- Run `idf.py fullclean` and rebuild

---

## 📊 CSV Output

New columns added:
```csv
...,latitude,longitude,speed_ms,heading_deg,gps_satellites,flag_manual,flag_auto
...,37.12345678,-122.98765432,5.23,45.67,8,0,1
```

---

## 📚 Full Documentation

- **Complete Guide**: `docs/gps-buzzer-integration-guide.md`
- **Development Note**: `docs/note-6-gps-buzzer-components.md`
- **Wiring Diagram**: `docs/complete-wiring-diagram.md`
- **Summary**: `GPS-BUZZER-SUMMARY.md`

---

**That's it! You're ready to integrate GPS and buzzer.**

**Estimated Time**: 1-2 hours for integration + testing
