# OLED Display Guide

**Last Updated**: March 24, 2026

---

## Overview

The ProxiTof system uses a 128×64 SSD1306 OLED display with 4-wire SPI interface. The display features dual-color zones (yellow top 16px, blue bottom 48px) and supports 9 display modes.

### Hardware Specifications
- **Display**: SSD1306 128×64 OLED, 0.96" yellow-blue dual color
- **Module**: 6-pin (GND, VCC, SCL, SDA, RES, DC)
- **Interface**: 4-wire SPI (no CS pin)
- **Colors**: Yellow (top 16px, pages 0-1) + Blue (bottom 48px, pages 2-7)
- **Library**: nopnop2002/esp-idf-ssd1306
- **Frame buffer**: 1024 bytes with differential updates (flicker-free)

---

## Wiring

### SPI Connections (from `oled_display.c`)

| Module Label | ESP32-S3 GPIO | SPI Function | SSD1306 Pin |
|--------------|---------------|--------------|-------------|
| GND | GND | Ground | GND |
| VCC | 3.3V | Power (3.3V only) | VCC_IN |
| SCL | GPIO 7 | SPI Clock (SCK) | D0 |
| SDA | GPIO 1 | SPI Data (MOSI) | D1 |
| RES | GPIO 21 | Reset | RES |
| DC | GPIO 35 | Data/Command | D/C |

> **Note on confusing pin labels:** This 6-pin 0.96" SSD1306 module has "SCL" and "SDA" printed on the PCB, which are typically I2C names. However, this module runs in 4-wire SPI mode. "SCL" is actually the SPI clock (D0 on the SSD1306 chip) and "SDA" is actually SPI MOSI data (D1 on the SSD1306 chip). There is no CS pin on this module. Do not confuse these with I2C — this is a pure SPI connection.

---

## Display Modes

### Mode 0: Dashboard (Default)
Full overview with dithered depth map, IMU summary, and status bar.
```
┌─────────────────────────┐
│ ProxiTof    12:34:56    │ ← Yellow zone (status bar + time)
│ LOG 3D 450mm            │ ← Logging/GPS/min distance
├─────────────────────────┤
│ ░░▒▒▓▓██ ░░▒▒▓▓██      │ ← Dithered depth map (4x4 zones)
│ ░░▒▒▓▓██ ░░▒▒▓▓██      │   8 density levels, EMA smoothed
│ ░░▒▒▓▓██ ░░▒▒▓▓██      │   Crosshair on closest zone
│ ░░▒▒▓▓██ ░░▒▒▓▓██      │   Dotted separator lines
│ R:+12 P:-5 Y:89        │ ← IMU roll/pitch/yaw
│ A:0.1 -0.0 9.8         │ ← IMU acceleration
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
1. Check SPI wiring (all 6 connections: GND, VCC, SCL, SDA, RES, DC)
2. Verify `OLED_ENABLED=true` in config.txt
3. Check power supply (3.3V, not 5V)
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

- `components/oled_display/oled_display.c` - Display driver (frame buffer, depth map, all modes)
- `components/oled_display/include/oled_display.h` - API definitions
- `components/oled_display/proxitof_logo.h` - Boot logo bitmap
- `managed_components/ssd1306/` - SSD1306 library (nopnop2002)

---

## Key Implementation Details

### Depth Map (Dashboard Mode)
- 4x4 dithered blocks (32x8px each) mapped to ToF zones
- 8 density levels: solid → dense checkerboard → sparse dots → empty
- EMA filter (alpha=0.7) smooths sudden distance jumps
- XOR crosshair marks the closest zone
- XOR dotted separator lines between columns

### Frame Buffer
- 1024-byte frame buffer with differential updates
- Only changed pages are sent to display (reduces SPI traffic)
- Eliminates flicker during updates

---

## Related Documentation

- [complete-wiring-diagram.md](complete-wiring-diagram.md) - Full hardware wiring
- [gpio-pin-reference.md](gpio-pin-reference.md) - GPIO assignments
- [display-modes-and-layouts.md](display-modes-and-layouts.md) - Detailed mode layouts

---

## Summary

The OLED display provides real-time feedback for all system functions:
- 9 display modes for different use cases
- Dual-color zones for visual hierarchy
- Button-controlled mode switching
- Context-aware ACTION button
- 2 Hz update rate for smooth visualization
