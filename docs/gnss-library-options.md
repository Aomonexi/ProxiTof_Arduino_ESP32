# GNSS Library Options for NEO-M9N-00B-00

**Date:** December 9, 2025  
**Module:** u-blox NEO-M9N-00B-00 GNSS receiver

## ESP-IDF Component Registry Options

### Option 1: espressif/nmea_parser ⭐ (Recommended)
**Component:** `espressif/nmea_parser`  
**Link:** https://components.espressif.com/components/espressif/nmea_parser

**Features:**
- ✅ Official Espressif component
- ✅ Parses NMEA sentences (standard GPS protocol)
- ✅ Works with any GNSS module including NEO-M9N
- ✅ Supports GGA, RMC, GSA, GSV sentences
- ✅ Easy to use API
- ✅ Well documented

**Usage:**
```yaml
# idf_component.yml
dependencies:
  espressif/nmea_parser: "^1.0.0"
```

**Code Example:**
```c
#include "nmea_parser.h"

// Initialize NMEA parser
nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
config.uart.uart_port = UART_NUM_1;
config.uart.rx_pin = GPIO_NUM_16;
config.uart.baud_rate = 9600;

nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);

// Register event handler
nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

// In event handler:
void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, 
                       int32_t event_id, void *event_data)
{
    gps_t *gps = (gps_t *)event_data;
    
    ESP_LOGI(TAG, "Lat: %.6f, Lon: %.6f, Alt: %.2f", 
             gps->latitude, gps->longitude, gps->altitude);
}
```

### Option 2: TinyGPS++ (Arduino-style)
**Component:** Not in ESP-IDF registry, but can be ported  
**Link:** https://github.com/mikalhart/TinyGPSPlus

**Features:**
- ✅ Lightweight NMEA parser
- ✅ Easy to understand
- ⚠️ Requires manual porting to ESP-IDF
- ⚠️ Arduino-style API

### Option 3: minmea (Minimal NMEA parser)
**Component:** Can be added as custom component  
**Link:** https://github.com/kosma/minmea

**Features:**
- ✅ Very lightweight
- ✅ Pure C implementation
- ✅ Easy to integrate
- ⚠️ Manual integration needed

### Option 4: u-blox UBX Protocol (Advanced)
**Component:** Custom implementation needed  
**Protocol:** UBX binary protocol (more efficient than NMEA)

**Features:**
- ✅ More efficient than NMEA
- ✅ More configuration options
- ✅ Better for high-rate updates
- ⚠️ More complex to implement
- ⚠️ Requires UBX protocol knowledge

## Recommendation for ProxiTof

### Use: `espressif/nmea_parser` ✅

**Reasons:**
1. **Official support** - Maintained by Espressif
2. **Easy integration** - Just add to `idf_component.yml`
3. **Standard protocol** - NEO-M9N outputs NMEA by default
4. **Proven** - Used in many ESP-IDF projects
5. **Event-driven** - Fits our FreeRTOS architecture

## NEO-M9N-00B-00 Specifications

**Communication:**
- UART (default 9600 baud, configurable up to 921600)
- I2C (optional, address 0x42)
- SPI (optional)

**Default Output:**
- NMEA 0183 sentences
- Update rate: 1Hz (configurable up to 25Hz)

**NMEA Sentences:**
- GGA: Position fix data
- RMC: Recommended minimum data
- GSA: Satellite status
- GSV: Satellites in view
- VTG: Track and ground speed

## Integration Plan for ProxiTof

### Hardware Connection

```
NEO-M9N-00B-00 → ESP32-S3
---------------------------------
VCC (3.3V)     → 3.3V
GND            → GND
TX             → GPIO 16 (UART RX)
RX             → GPIO 17 (UART TX)
```

### Component Configuration

**File:** `components/sensor_manager/idf_component.yml`
```yaml
dependencies:
  rjrp44/vl53l8cx: "^4.0.0"
  espressif/nmea_parser: "^1.0.0"
```

### Code Integration

**In sensor_manager.h:**
```c
typedef struct {
    uint32_t timestamp_ms;
    double latitude;        // Degrees
    double longitude;       // Degrees
    float altitude;         // Meters
    float speed;            // km/h
    uint8_t satellites;     // Number of satellites
    bool valid;             // GPS fix valid
} gnss_data_t;

esp_err_t sensor_manager_read_gnss(gnss_data_t *data);
```

**In sensor_manager.c:**
```c
#include "nmea_parser.h"

static nmea_parser_handle_t nmea_hdl = NULL;
static gnss_data_t last_gnss_data = {0};

static void gnss_event_handler(void *arg, esp_event_base_t base, 
                                int32_t id, void *data)
{
    gps_t *gps = (gps_t *)data;
    
    last_gnss_data.timestamp_ms = sensor_manager_get_timestamp();
    last_gnss_data.latitude = gps->latitude;
    last_gnss_data.longitude = gps->longitude;
    last_gnss_data.altitude = gps->altitude;
    last_gnss_data.speed = gps->speed;
    last_gnss_data.satellites = gps->sats_in_use;
    last_gnss_data.valid = (gps->fix > 0);
}

static esp_err_t init_gnss_module(void)
{
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    config.uart.uart_port = UART_NUM_1;
    config.uart.rx_pin = GPIO_NUM_16;
    config.uart.tx_pin = GPIO_NUM_17;
    config.uart.baud_rate = 9600;
    
    nmea_hdl = nmea_parser_init(&config);
    if (nmea_hdl == NULL) {
        return ESP_FAIL;
    }
    
    nmea_parser_add_handler(nmea_hdl, gnss_event_handler, NULL);
    return ESP_OK;
}

esp_err_t sensor_manager_read_gnss(gnss_data_t *data)
{
    if (data == NULL || nmea_hdl == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(data, &last_gnss_data, sizeof(gnss_data_t));
    return ESP_OK;
}
```

## CSV Format Update

When GNSS is enabled, add these columns:

```csv
...,flag_manual,flag_auto,confidence,latitude,longitude,altitude,speed,satellites
```

## Performance Considerations

**Update Rate:**
- GNSS: 1Hz (default) - sufficient for location tracking
- ToF: 60Hz - primary sensor
- IMU: 100Hz - primary sensor

**Data Volume Impact:**
- Additional ~40 bytes per row
- Minimal impact on SD card storage
- Only updated when GPS fix is valid

## Summary

✅ **Use `espressif/nmea_parser`** - Official, easy, reliable  
✅ **UART connection** - Simple 2-wire interface (TX/RX)  
✅ **Event-driven** - Fits our architecture  
✅ **1Hz updates** - Sufficient for location tracking  
✅ **Easy integration** - Add to `idf_component.yml`

The NEO-M9N will work perfectly with ESP-IDF using the official NMEA parser component!
