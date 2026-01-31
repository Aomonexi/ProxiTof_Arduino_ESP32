# OLED Display - Complete Guide

**Consolidated Documentation**  
**Original Dates**: January 25-30, 2026  
**Last Updated**: January 31, 2026

---

## Overview

The ProxiTof system uses a 128×64 SSD1306 OLED display with SPI interface. The display features dual-color zones (yellow top, blue bottom) and supports 9 different display modes.

### Hardware Specifications
- **Display**: SSD1306 128×64 OLED
- **Interface**: 4-wire SPI
- **Colors**: Yellow (top 16px) + Blue (bottom 48px)
- **Library**: nopnop2002/esp-idf-ssd1306

---

## Wiring

### SPI Connections
| OLED Pin | ESP32-S3 GPIO | Function |
|----------|---------------|----------|
| GND | GND | Ground |
| VCC | 3.3V | Power |
| SCL/CLK | GPIO 12 | SPI Clock |
| SDA/MOSI | GPIO 11 | SPI Data |
| RES | GPIO 9 | Reset |
| DC | GPIO 13 | Data/Command |
| CS | GPIO 10 | Chip Select |

### Configuration (sdkconfig)
```
CONFIG_SPI2_HOST=y
CONFIG_MOSI_GPIO=11
CONFIG_SCLK_GPIO=12
CONFIG_CS_GPIO=10
CONFIG_DC_GPIO=13
CONFIG_RESET_GPIO=9
```

---

## Display Modes

### Mode 0: Dashboard (Default)
Full overview with ToF grid, IMU data, and statistics.
```
┌─────────────────────────┐
│ ProxiTof    12:34:56    │ ← Yellow zone (status bar)
├─────────────────────────┤
│ ToF Grid    │ IMU Data  │ ← Blue zone
│ [4x4 bars]  │ Roll/Pitch│
│             │ Accel     │
│ Stats: Near-miss count  │
└─────────────────────────┘
```

### Mode 1: Status
System health and component status.
```
┌─────────────────────────┐
│ System Status           │
├─────────────────────────┤
│ ToF: ✓  IMU: ✓  GPS: ✓  │
│ SD: ✓   RTC: ✓  WiFi: ✗ │
│ Uptime: 01:23:45        │
│ Free RAM: 123KB         │
└─────────────────────────┘
```

### Mode 2: ToF Focus
Large 4×4 distance grid with bar graphs.
```
┌─────────────────────────┐
│ ToF Focus               │
├─────────────────────────┤
│ ████ ███  ██   █        │
│ ████ ████ ███  ██       │
│ ███  ██   █    █        │
│ ██   █    █    █        │
│ Min: 45cm  Max: 200cm   │
└─────────────────────────┘
```

### Mode 3: IMU Focus
Detailed orientation and acceleration data.
```
┌─────────────────────────┐
│ IMU Focus               │
├─────────────────────────┤
│ Roll:  +12.3°           │
│ Pitch: -5.7°            │
│ Yaw:   +45.2°           │
│ Accel: X:0.1 Y:0.0 Z:1.0│
│ Gyro:  X:0.0 Y:0.1 Z:0.0│
└─────────────────────────┘
```

### Mode 4: GPS
GPS coordinates and satellite information.
```
┌─────────────────────────┐
│ GPS Mode                │
├─────────────────────────┤
│ Lat: 52.5200° N         │
│ Lon: 13.4050° E         │
│ Alt: 34.5m              │
│ Sats: 8  HDOP: 1.2      │
│ Speed: 15.3 km/h        │
└─────────────────────────┘
```

### Mode 5: Near-Miss
Near-miss detection status and mode.
```
┌─────────────────────────┐
│ Near-Miss Detection     │
├─────────────────────────┤
│ Mode: Hybrid            │ ← Press ACTION to cycle
│ Status: Monitoring      │
│ Count: 5 events         │
│ Last: 2 min ago         │
│ ML: ✓ Ready             │
└─────────────────────────┘
```

### Mode 6: WiFi Settings
WiFi configuration and time sync.
```
┌─────────────────────────┐
│ WiFi Settings           │
├─────────────────────────┤
│ SSID: MyNetwork         │
│ Status: Connected       │
│ IP: 192.168.1.100       │
│ Time Sync: ✓            │
│ Press ACTION to sync    │
└─────────────────────────┘
```

### Mode 7: USB Transfer
USB Mass Storage mode for file access.
```
┌─────────────────────────┐
│ !USB MODE ACTIVE!       │ ← Inverted when active
│ Logging PAUSED          │
├─────────────────────────┤
│                         │
│   [USB]                 │
│                         │
│ Connect LEFT port       │
│ ACTION to exit          │
└─────────────────────────┘
```

### Mode 8: Shutdown
Safe shutdown confirmation.
```
┌─────────────────────────┐
│ Shutdown Mode           │
├─────────────────────────┤
│                         │
│ Press ACTION to         │
│ safely shutdown         │
│                         │
│ Data will be saved      │
└─────────────────────────┘
```

---

## Button Controls

### Mode Button (GPIO 20)
- **Function**: Cycle through display modes
- **Debounce**: 200ms
- **Wiring**: Button between GPIO 20 and GND

### ACTION Button (GPIO 3)
Context-aware actions depending on current mode:

| Mode | ACTION Button Function |
|------|------------------------|
| Dashboard | Flag near-miss event |
| Status | Refresh status |
| ToF Focus | - |
| IMU Focus | Rescan IMU |
| GPS | Reconnect GPS |
| Near-Miss | Cycle detection mode |
| WiFi Settings | Sync time |
| USB Transfer | Toggle USB MSC |
| Shutdown | Confirm shutdown |

---

## Dual-Color Zones

The SSD1306 display has two color zones:

### Yellow Zone (Pages 0-1, 16 pixels)
- Status bar, headers, warnings
- High visibility for important info
- Inverted (white on black) for alerts

### Blue Zone (Pages 2-7, 48 pixels)
- Main content area
- Data visualization
- Detailed information

---

## Display Update Rate

- **Normal operation**: 2 Hz (500ms interval)
- **During transitions**: Immediate update
- **USB mode**: Static display

---

## Troubleshooting

### Display Not Working
1. Check SPI wiring (all 7 connections)
2. Verify `OLED_ENABLED=true` in config.txt
3. Check power supply (3.3V)
4. Try different GPIO pins if conflicts

### Display Glitches
1. Add small delay after mode switch
2. Check for SPI bus conflicts
3. Reduce update rate if needed

### Inverted Colors
- Normal: White text on black background
- Inverted headers indicate active/warning state

### Flickering
1. Check SPI clock speed
2. Verify stable power supply
3. Add decoupling capacitor near display

---

## Configuration

### Enable/Disable OLED
In `data/config.txt`:
```
OLED_ENABLED=true
```

### Display Timeout (Optional)
```
OLED_TIMEOUT_SEC=300  # Turn off after 5 minutes
```

---

## Implementation Files

- `components/oled_display/oled_display.c` - Display driver
- `components/oled_display/include/oled_display.h` - API definitions
- `components/oled_display/proxitof_logo.h` - Boot logo
- `managed_components/ssd1306/` - SSD1306 library

---

## Related Documentation

- `docs/complete-wiring-diagram.md` - Full hardware wiring
- `docs/GPIO_PIN_REFERENCE.md` - GPIO assignments
- `docs/note-7-OLED_RESOLUTION_FINAL.md` - Resolution details

---

## Summary

The OLED display provides real-time feedback for all system functions:
- 9 display modes for different use cases
- Dual-color zones for visual hierarchy
- Button-controlled mode switching
- Context-aware ACTION button
- 2 Hz update rate for smooth visualization
