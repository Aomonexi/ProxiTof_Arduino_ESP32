# OLED Status Bar & Symmetric Bars Update

**Date**: January 26, 2026  
**Status**: ✅ Improved - Status bar shows time/GPS/logging, bars are symmetric

---

## Changes Made

### 1. Enhanced Status Bar (Yellow Zone) ✅

**Before:**
```
Min:123mm 60Hz
```

**After:**
```
12:34:56 LOG GPS    ← Line 1: Time, Logging status, GPS status
Min:123mm           ← Line 2: Minimum distance
```

**Features:**
- **Time**: Real-time clock (HH:MM:SS format)
- **Logging Status**: "LOG" when active, "---" when stopped
- **GPS Status**: "GPS" indicator (can be expanded to show fix quality)
- **Min Distance**: Closest object in ToF grid

---

### 2. Symmetric Bar Layout ✅

**Before:**
- Bars: 30px wide, uneven spacing
- Position: `col * 31 + 2` (asymmetric)
- Started at page 3

**After:**
- Bars: 28px wide × 6px tall
- Position: `col * 32` (perfectly symmetric!)
- Spacing: 4px between bars
- Started at page 2 (more room)

**Layout Math:**
```
Display width: 128 pixels
Bars per row: 4
Bar width: 28 pixels
Spacing: 4 pixels

Total per bar: 28 + 4 = 32 pixels
Total width: 32 × 4 = 128 pixels ✅ Perfect fit!
```

**Visual:**
```
┌────────────────────────────────────────┐
│ 12:34:56 LOG GPS                       │ ← Page 0
│ Min:123mm                              │ ← Page 1
├────────────────────────────────────────┤
│ [■■■■■■] [■■■■  ] [■■    ] [■      ]  │ ← Page 2 (Row 1)
│ [■■■■■■] [■■■■■■] [■■■■  ] [■■    ]  │ ← Page 3 (Row 2)
│ [■■■■  ] [■■■■■■] [■■■■■■] [■■■■  ]  │ ← Page 4 (Row 3)
│ [■■    ] [■■■■  ] [■■■■■■] [■■■■■■]  │ ← Page 5 (Row 4)
│                                        │ ← Page 6 (empty)
│ R+12.3 P-5.4 Y180.0                    │ ← Page 7 (IMU)
└────────────────────────────────────────┘
```

---

## Status Bar Details

### Time Display

**Source:** Smart Time Management System
- DS3231 RTC (battery-backed)
- WiFi/NTP sync
- Internal RTC fallback

**Format:** `HH:MM:SS` (24-hour)
- Example: `12:34:56`
- Updates every second (but display refreshes at 20 Hz)

**Fallback:** `??:??:??` if time not available

---

### Logging Status

**Source:** Storage Manager
- Checks `storage_stats.logging_active`

**Display:**
- `LOG` - Logging is active (writing to SD card)
- `---` - Logging is stopped

**Why useful:**
- Confirms data is being saved
- Quick visual check during testing
- Alerts if logging stops unexpectedly

---

### GPS Status

**Current:** Simple `GPS` indicator

**Future Enhancements:**
```c
// TODO: Expand GPS status display
gps_data_t gps_data;
if (gps_read(&gps_data) == ESP_OK && gps_data.fix_quality > 0) {
    snprintf(gps_status, sizeof(gps_status), "G%d", gps_data.satellites);
    // Shows: G8 (8 satellites)
} else {
    snprintf(gps_status, sizeof(gps_status), "G-");
    // Shows: G- (no fix)
}
```

**Possible Status Indicators:**
- `G-` - No GPS fix
- `G3` - GPS fix with 3 satellites (poor)
- `G8` - GPS fix with 8 satellites (good)
- `G12` - GPS fix with 12 satellites (excellent)

---

## Bar Graph Improvements

### Symmetric Layout

**Key Changes:**
1. **Even spacing**: 32 pixels per column (28 bar + 4 spacing)
2. **Centered**: Bars start at x=0, perfectly aligned
3. **Consistent height**: 6 pixels tall (leaves 2 pixels for border)
4. **More room**: Started at page 2 instead of 3

**Benefits:**
- ✅ Visually balanced
- ✅ Easier to compare bars
- ✅ Professional appearance
- ✅ No wasted space

---

### Bar Drawing Function

**Updated Signature:**
```c
static void draw_bar_from_left(SSD1306_t *dev, 
                               int page,      // Page number (0-7)
                               int x,         // X position
                               int y_offset,  // Y offset within page
                               int width,     // Bar width (28px)
                               int height,    // Bar height (6px)
                               uint16_t value,      // Distance (mm)
                               uint16_t max_value); // Max distance (2000mm)
```

**Why y_offset parameter?**
- Allows precise vertical positioning within a page
- Bars can be centered vertically
- More flexible layout options

**Example:**
```c
// Draw bar at page 2, x=0, y_offset=2 (2 pixels from top)
draw_bar_from_left(&dev, 2, 0, 2, 28, 6, distance, 2000);
```

---

## Code Structure

### Includes Added

```c
#include "smart_time.h"      // For time display
#include "storage_manager.h" // For logging status
#include <time.h>            // For struct tm, strftime
```

### Dashboard Update Flow

```c
1. Get current time from smart_time_get()
2. Get logging status from storage_manager_get_stats()
3. Format status line 1: "HH:MM:SS LOG GPS"
4. Format status line 2: "Min:XXXmm"
5. Display status lines in yellow zone (pages 0-1)
6. Draw 4x4 ToF bar grid in blue zone (pages 2-5)
7. Display IMU summary in blue zone (page 7)
```

---

## Display Layout Breakdown

### Page Allocation

| Page | Height | Content | Zone |
|------|--------|---------|------|
| 0 | 8px | Time + Log + GPS | Yellow |
| 1 | 8px | Min distance | Yellow |
| 2 | 8px | ToF Row 1 (4 bars) | Blue |
| 3 | 8px | ToF Row 2 (4 bars) | Blue |
| 4 | 8px | ToF Row 3 (4 bars) | Blue |
| 5 | 8px | ToF Row 4 (4 bars) | Blue |
| 6 | 8px | Empty (reserved) | Blue |
| 7 | 8px | IMU orientation | Blue |

**Total:** 64 pixels (8 pages × 8 pixels)

---

### Pixel Budget

**Yellow Zone (16 pixels):**
- Page 0: Time, logging, GPS status
- Page 1: Minimum distance

**Blue Zone (48 pixels):**
- Pages 2-5: 4×4 ToF bar grid (32 pixels)
- Page 6: Reserved for future use (8 pixels)
- Page 7: IMU orientation summary (8 pixels)

---

## Example Status Displays

### Normal Operation
```
12:34:56 LOG GPS
Min:234mm
```

### No GPS Fix
```
12:34:56 LOG G-
Min:234mm
```

### Logging Stopped
```
12:34:56 --- GPS
Min:234mm
```

### Time Not Available
```
??:??:?? LOG GPS
Min:234mm
```

---

## Future Enhancements

### 1. GPS Fix Quality
```c
// Show satellite count
"12:34:56 LOG G8"  // 8 satellites

// Or show fix quality
"12:34:56 LOG G*"  // Good fix
"12:34:56 LOG G?"  // Poor fix
"12:34:56 LOG G-"  // No fix
```

### 2. Battery Status
```c
// If running on battery
"12:34:56 LOG GPS 85%"  // 85% battery
```

### 3. Event Counter
```c
// Show near-miss event count
"12:34:56 LOG GPS E:5"  // 5 events logged
```

### 4. Data Rate
```c
// Show logging rate
"12:34:56 LOG GPS 60Hz"  // 60 samples/sec
```

---

## Testing

### Verify Status Bar

**1. Check Time Display:**
```
- Should update every second
- Format: HH:MM:SS (24-hour)
- Shows ??:??:?? if time not set
```

**2. Check Logging Status:**
```
- Shows "LOG" when storage_manager_is_logging() returns true
- Shows "---" when logging is stopped
- Toggle logging to verify
```

**3. Check GPS Status:**
```
- Currently shows "GPS" always
- TODO: Implement actual GPS fix status
```

### Verify Bar Symmetry

**1. Visual Check:**
```
- All bars should be same width (28px)
- Equal spacing between bars (4px)
- Bars should align vertically
- No gaps or overlaps
```

**2. Measure:**
```
- Bar 1: x=0 to x=27 (28px)
- Bar 2: x=32 to x=59 (28px)
- Bar 3: x=64 to x=91 (28px)
- Bar 4: x=96 to x=123 (28px)
- Total: 128px ✅
```

---

## Summary

**Status Bar (Yellow Zone):**
- ✅ Shows time (HH:MM:SS)
- ✅ Shows logging status (LOG/---)
- ✅ Shows GPS indicator
- ✅ Shows minimum distance

**Bar Layout (Blue Zone):**
- ✅ Symmetric 4×4 grid
- ✅ 28px wide bars
- ✅ 4px spacing
- ✅ Perfect 128px fit
- ✅ Professional appearance

**Result:** Clean, informative dashboard with all key information at a glance!

---

**Last Updated:** January 26, 2026  
**Tested On:** ESP32-S3 with SSD1306 128x64 OLED
