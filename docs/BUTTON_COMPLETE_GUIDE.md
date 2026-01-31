# Button Behavior - Complete Guide

**Consolidated Documentation**  
**Original Dates**: January 25-29, 2026  
**Last Updated**: January 31, 2026

---

## Overview

The ProxiTof system uses two buttons for user interaction:
- **MODE Button** (GPIO 20): Cycle through display modes
- **ACTION Button** (GPIO 3): Context-aware actions

---

## Hardware Setup

### MODE Button (GPIO 20)
- **Type**: Normally Open (NO) momentary push button
- **Wiring**: Button between GPIO 20 and GND
- **Pull-up**: Internal (no external resistor needed)
- **Debounce**: 200ms in software

### ACTION Button (GPIO 3)
- **Type**: Normally Open (NO) momentary push button
- **Wiring**: Button between GPIO 3 and GND
- **Pull-up**: Internal (no external resistor needed)
- **Debounce**: 200ms in software

---

## Button Functions

### MODE Button (GPIO 20)
**Function**: Cycle through display modes (0-8)

```
Dashboard (0) → Status (1) → ToF Focus (2) → IMU Focus (3) → GPS (4) → 
Near Miss (5) → WiFi Settings (6) → USB Transfer (7) → Shutdown (8) → (back to 0)
```

**Behavior**:
- Single press cycles to next mode
- Wraps around from Mode 8 to Mode 0
- Visual feedback on OLED
- Serial log: "🔄 Display mode changed: X → Y"

---

### ACTION Button (GPIO 3)
**Function**: Context-aware actions depending on current mode

| Mode | Mode Name | ACTION Button Function | LED Feedback |
|------|-----------|------------------------|--------------|
| 0 | Dashboard | Flag near-miss event | Blue flash (100ms) |
| 1 | Status | Refresh status | - |
| 2 | ToF Focus | - | - |
| 3 | IMU Focus | Rescan IMU sensor | - |
| 4 | GPS | Reconnect GPS | - |
| 5 | Near-Miss | Cycle detection mode | - |
| 6 | WiFi Settings | Sync time via WiFi | - |
| 7 | USB Transfer | Toggle USB MSC on/off | Blue (solid) |
| 8 | Shutdown | Confirm safe shutdown | - |

---

## Detailed ACTION Button Behavior

### Mode 0 (Dashboard): Manual Event Flagging
**Purpose**: Mark near-miss events for ML training ground truth

**Behavior**:
1. Press ACTION button
2. LED flashes blue (100ms)
3. `flag_manual=1` written to current log entry
4. Data immediately flushed to SD card
5. Serial log: "🚨 Manual flag triggered"

**Use Case**: During data collection, press when a near-miss occurs

### Mode 3 (IMU Focus): IMU Rescan
**Purpose**: Re-initialize IMU sensor if issues detected

**Behavior**:
1. Press ACTION button
2. System attempts to rescan I2C bus
3. IMU re-initialized if found
4. Status updated on display

### Mode 4 (GPS): GPS Reconnect
**Purpose**: Attempt to reconnect GPS if signal lost

**Behavior**:
1. Press ACTION button
2. GPS module reset
3. Waits for new fix
4. Status updated on display

### Mode 5 (Near-Miss): Detection Mode Cycling
**Purpose**: Switch between detection algorithms

**Behavior**:
1. Press ACTION button
2. Mode cycles: Hybrid → Algorithm → Neural Network → Hybrid
3. New mode displayed on OLED
4. Mode saved to config (persists across power cycles)
5. Serial log: "Detection mode changed to: X"

**Detection Modes**:
- **Hybrid (0)**: Algorithm + ML (best accuracy)
- **Algorithm (1)**: Rule-based only (for data collection)
- **Neural Network (2)**: ML only (experimental)

### Mode 6 (WiFi Settings): Time Sync
**Purpose**: Synchronize time via WiFi NTP

**Behavior**:
1. Press ACTION button
2. System connects to configured WiFi
3. NTP time sync attempted
4. RTC updated if successful
5. Status displayed on OLED

### Mode 7 (USB Transfer): Toggle USB MSC
**Purpose**: Enable/disable USB Mass Storage mode

**Behavior**:
1. Press ACTION button
2. If USB MSC off:
   - Logging pauses
   - SD card mounts to USB
   - LED turns blue
   - Display shows "USB MODE ACTIVE"
3. If USB MSC on:
   - SD card unmounts from USB
   - Logging resumes
   - LED returns to normal
   - Display shows "USB MODE OFF"

### Mode 8 (Shutdown): Safe Shutdown
**Purpose**: Safely power down the device

**Behavior**:
1. Press ACTION button
2. All data flushed to SD card
3. File handles closed
4. System enters deep sleep
5. Safe to remove power

---

## LED Feedback Summary

| Action | LED Response |
|--------|--------------|
| Manual flag (Dashboard) | Blue flash (100ms) |
| USB mode active | Blue (solid) |
| Near-miss detected | Orange flash |
| Normal operation | Distance-based color |

### Distance-Based LED Colors
| Distance | LED Color |
|----------|-----------|
| > 150cm | Green |
| 100-150cm | Yellow |
| 50-100cm | Orange |
| < 50cm | Red |

---

## Debouncing

Both buttons use software debouncing:
- **Debounce time**: 200ms
- **Implementation**: Timestamp comparison
- **Prevents**: False triggers from button bounce

---

## Troubleshooting

### Button Not Responding
1. Check wiring (button between GPIO and GND)
2. Verify internal pull-up enabled
3. Check for GPIO conflicts
4. Test with multimeter

### Multiple Triggers on Single Press
1. Increase debounce time
2. Check for loose connections
3. Replace button if worn

### Wrong Action Triggered
1. Verify current display mode
2. Check button GPIO assignments
3. Review serial logs for mode state

---

## GPIO Configuration

```c
// In main.c
#define MODE_BUTTON_PIN     GPIO_NUM_20
#define ACTION_BUTTON_PIN   GPIO_NUM_3
#define BUTTON_DEBOUNCE_MS  200

// Button initialization
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << MODE_BUTTON_PIN) | (1ULL << ACTION_BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};
gpio_config(&io_conf);
```

---

## Related Documentation

- `docs/GPIO_PIN_REFERENCE.md` - Complete GPIO assignments
- `docs/OLED_COMPLETE_GUIDE.md` - Display modes
- `docs/USB_COMPLETE_GUIDE.md` - USB Transfer mode details
- `docs/ML_COMPLETE_GUIDE.md` - Detection modes and ML

---

## Summary

| Button | GPIO | Function |
|--------|------|----------|
| MODE | 20 | Cycle display modes (0-8) |
| ACTION | 3 | Context-aware actions |

**Key Points**:
- MODE button always cycles modes
- ACTION button behavior depends on current mode
- Dashboard mode: ACTION flags events (for ML training)
- Near-Miss mode: ACTION cycles detection modes
- USB Transfer mode: ACTION toggles USB MSC
