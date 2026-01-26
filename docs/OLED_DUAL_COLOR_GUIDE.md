# OLED Dual-Color Display Guide

**Display**: SSD1306 128x64 with Yellow/Blue zones  
**Yellow Zone**: Top 16 pixels (rows 0-15)  
**Blue Zone**: Bottom 48 pixels (rows 16-63)

---

## Perfect Use Cases for Dual-Color Zones

### Yellow Zone (Top 16 pixels) - Alerts & Status
**Why Yellow**: Attention-grabbing, perfect for warnings and important info

**Ideal Content:**
- ⚠️ **Near-Miss Alerts** - "NEAR MISS!" flashing text
- 🔴 **Critical Proximity** - "TOO CLOSE!" warning
- 📊 **System Status** - "LOGGING" / "USB MODE" / "ERROR"
- 🔋 **Battery/Power** - Power status indicator
- 📡 **Connection Status** - WiFi, GPS, SD card status
- ⏱️ **Time/Date** - Current time display

### Blue Zone (Bottom 48 pixels) - Data & Visualization
**Why Blue**: Calm, easy to read, perfect for data

**Ideal Content:**
- 📏 **ToF Grid** - 4x4 distance visualization
- 🧭 **IMU Data** - Orientation, acceleration, gyro
- 📈 **Graphs** - Real-time distance/speed plots
- 🔢 **Numeric Data** - Sensor readings with units
- 📊 **Statistics** - Frame rate, buffer usage, event count
- 🗺️ **GPS Data** - Coordinates, speed, heading

---

## Recommended Display Layouts

### Layout 1: Dashboard Mode (Default)
```
┌────────────────────────────────┐
│ YELLOW: LOGGING  12:34:56  GPS │ ← Status bar
├────────────────────────────────┤
│ BLUE:                          │
│  ToF Grid:    IMU:             │
│  [234][456]   Roll: +12.3°     │
│  [123][567]   Pitch: -5.4°     │
│  [890][234]   Yaw: 180.0°      │
│  [456][123]                    │
│  Min: 123mm   Accel: 0.5m/s²   │
└────────────────────────────────┘
```

### Layout 2: Near-Miss Alert Mode
```
┌────────────────────────────────┐
│ YELLOW: ⚠️ NEAR MISS! ⚠️       │ ← Flashing alert
├────────────────────────────────┤
│ BLUE:                          │
│  Distance: 234mm               │
│  Approach: -500mm/s            │
│  Jerk: 2.5m/s³                 │
│                                │
│  [Event logged at 12:34:56]    │
│                                │
└────────────────────────────────┘
```

### Layout 3: ToF Focus Mode
```
┌────────────────────────────────┐
│ YELLOW: Min: 123mm  -450mm/s   │ ← Key metrics
├────────────────────────────────┤
│ BLUE: ToF 4x4 Grid (Large)     │
│                                │
│   234  456  789  123           │
│   567  890  234  456           │
│   789  123  456  567           │
│   890  234  567  789           │
│                                │
└────────────────────────────────┘
```

### Layout 4: IMU Focus Mode
```
┌────────────────────────────────┐
│ YELLOW: IMU  Jerk: 1.2m/s³     │ ← IMU status
├────────────────────────────────┤
│ BLUE:                          │
│  Orientation:                  │
│    Roll:  +12.34°              │
│    Pitch:  -5.67°              │
│    Yaw:  180.00°               │
│                                │
│  Acceleration:                 │
│    X: +0.12 m/s²               │
│    Y: -0.34 m/s²               │
│    Z: +9.81 m/s²               │
└────────────────────────────────┘
```

### Layout 5: Status/Debug Mode
```
┌────────────────────────────────┐
│ YELLOW: ProxiTof v1.0  OK      │ ← System info
├────────────────────────────────┤
│ BLUE:                          │
│  ToF: OK  60Hz                 │
│  IMU: OK  100Hz                │
│  SD:  OK  512MB free           │
│  GPS: FIX  8 sats              │
│  RTC: 2026-01-25 12:34:56      │
│  Buf: 45% (54/120)             │
│  Logs: 1,234 events            │
└────────────────────────────────┘
```

---

## Color Zone Pixel Mapping

### Yellow Zone (Pages 0-1)
```
Page 0: Rows 0-7   (pixels 0-7)
Page 1: Rows 8-15  (pixels 8-15)
Total: 16 pixels height
```

### Blue Zone (Pages 2-7)
```
Page 2: Rows 16-23 (pixels 16-23)
Page 3: Rows 24-31 (pixels 24-31)
Page 4: Rows 32-39 (pixels 32-39)
Page 5: Rows 40-47 (pixels 40-47)
Page 6: Rows 48-55 (pixels 48-55)
Page 7: Rows 56-63 (pixels 56-63)
Total: 48 pixels height
```

---

## Implementation Tips

### 1. Use Yellow for Attention
```c
// Flash warning in yellow zone
void display_warning(const char *text) {
    // Yellow zone = pages 0-1 (top 16 pixels)
    ssd1306_display_text(dev, 0, text, true);  // Inverted for emphasis
}
```

### 2. Use Blue for Data
```c
// Display sensor data in blue zone
void display_sensor_data(tof_data_t *tof, imu_data_t *imu) {
    // Blue zone = pages 2-7 (bottom 48 pixels)
    ssd1306_display_text(dev, 2, "ToF Grid:", false);
    // ... display grid data
}
```

### 3. Separate Concerns
```
Yellow = What's happening NOW (alerts, status)
Blue = What's the DATA (measurements, readings)
```

### 4. Animation Ideas
```c
// Flashing alert in yellow zone
for (int i = 0; i < 6; i++) {
    ssd1306_display_text(dev, 0, "⚠️ ALERT! ⚠️", true);
    vTaskDelay(pdMS_TO_TICKS(250));
    ssd1306_clear_page(dev, 0);
    vTaskDelay(pdMS_TO_TICKS(250));
}
```

---

## Recommended Display Modes

### Mode 0: Dashboard (Default)
- **Yellow**: Status bar (logging, time, GPS)
- **Blue**: ToF grid + IMU summary

### Mode 1: ToF Focus
- **Yellow**: Min distance + approach rate
- **Blue**: Large 4x4 ToF grid

### Mode 2: IMU Focus
- **Yellow**: Jerk magnitude + status
- **Blue**: Full IMU data (orientation + accel + gyro)

### Mode 3: Status/Debug
- **Yellow**: System name + overall status
- **Blue**: Component status + statistics

### Mode 4: Near-Miss Alert (Auto-trigger)
- **Yellow**: Flashing "NEAR MISS!" warning
- **Blue**: Event details (distance, approach, jerk)

---

## Visual Hierarchy

**Priority 1 (Yellow)**: Critical alerts, warnings, status  
**Priority 2 (Blue)**: Detailed data, measurements, graphs

**Example Flow:**
1. Normal operation: Yellow shows "LOGGING", Blue shows sensor data
2. Near-miss detected: Yellow flashes "NEAR MISS!", Blue shows event details
3. After 3 seconds: Return to normal display

---

## Font Sizes for Each Zone

### Yellow Zone (16 pixels height)
- **8x8 font**: 2 lines of text (perfect!)
- **8x16 font**: 1 line of large text

### Blue Zone (48 pixels height)
- **8x8 font**: 6 lines of text
- **8x16 font**: 3 lines of large text
- **Custom graphics**: ToF grid, graphs, icons

---

## Summary

**Yellow Zone**: Attention-grabbing alerts and status  
**Blue Zone**: Calm, detailed data visualization  

**Perfect for your near-miss detection system!**
- Yellow warns you when something happens
- Blue shows you what's happening in detail

This dual-color feature is a huge advantage - use it wisely! 🎨
