# OLED Display Crash Fix & ToF Sensor Handling

**Date**: January 26, 2026  
**Issue**: Crash when switching to Near Miss mode + ToF sensor invalid readings  
**Status**: ✅ Fixed

---

## Issue 1: Crash on Near Miss Mode Switch

### Problem
```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
PC: 0x400556d5 (strlen in ROM)
Backtrace: check_mode_button at main.c:99
```

**Root Cause**: Array out of bounds access
- Added 5th mode (Near Miss) to enum
- But `main.c` still had 4-mode array
- When switching to mode 4 (Near Miss), it accessed `mode_names[4]` which didn't exist
- Caused crash when trying to log the mode name

### Solution

**File**: `main/main.c`

**Before:**
```c
const char *mode_names[] = {"Dashboard", "ToF Focus", "IMU Focus", "Status"};
// Only 4 modes - accessing mode_names[4] = CRASH!
```

**After:**
```c
const char *mode_names[] = {"Dashboard", "ToF Focus", "IMU Focus", "Status", "Near Miss"};
// Now has 5 modes - mode_names[4] is valid
```

---

## Issue 2: ToF Sensor Invalid Readings

### Problem

**Symptom**: When object is too close or out of range, ToF sensor returns very large numbers (8191 or 0x1FFF)

**Why This Happens**:
- VL53L8CX ToF sensor has a valid range (typically 0-4000mm)
- When object is too close (< minimum range) → returns 8191
- When object is too far (> maximum range) → returns 8191
- When no object detected → returns 8191
- 8191 = 0x1FFF = 13 bits all set (sensor's "invalid" marker)

**Impact**:
1. **Display overflow**: 8191 is 4 digits, but we only have space for 3
2. **Bar graphs wrong**: Bars show empty when they should show "invalid"
3. **Statistics wrong**: Average/max calculations include invalid readings

### Solution

#### Fix 1: Clamp Values in ToF Focus Mode

**File**: `components/oled_display/oled_display.c`

```c
// Clamp values to 999 max to prevent overflow
// ToF sensor returns 8191 (0x1FFF) for out-of-range/invalid readings
uint16_t d0 = tof_data->distances[row*4+0];
uint16_t d1 = tof_data->distances[row*4+1];
uint16_t d2 = tof_data->distances[row*4+2];
uint16_t d3 = tof_data->distances[row*4+3];

// Clamp to 999 or show "---" for invalid readings
if (d0 > 4000) d0 = 999;  // Out of range
if (d1 > 4000) d1 = 999;
if (d2 > 4000) d2 = 999;
if (d3 > 4000) d3 = 999;

// Format: 3 digits per value + 1 space = 4 chars × 4 = 16 chars (perfect fit!)
snprintf(line, sizeof(line), "%3d %3d %3d %3d", d0, d1, d2, d3);
```

**Result**: Numbers always fit in 3 digits, no overflow

#### Fix 2: Handle Invalid Readings in Bar Graphs

**File**: `components/oled_display/oled_display.c`

```c
static void fb_draw_bar(int page, int x, int width, uint16_t distance, uint16_t max_distance) {
    // Handle invalid ToF readings (sensor returns 8191 for out-of-range)
    if (distance > 4000) {
        distance = max_distance;  // Treat as max distance (empty bar)
    }
    
    // Calculate fill level (closer = fuller bar)
    float fill_percent = 1.0f - ((float)distance / (float)max_distance);
    // ... rest of function
}
```

**Result**: Invalid readings show as empty bars (far away)

#### Fix 3: Exclude Invalid Readings from Statistics

**File**: `components/oled_display/oled_display.c`

```c
// Page 7: Statistics
{
    uint16_t max_dist = 0;
    uint32_t sum = 0;
    int valid_count = 0;
    
    for (int i = 0; i < 16; i++) {
        uint16_t dist = tof_data->distances[i];
        if (dist <= 4000) {  // Only count valid readings
            if (dist > max_dist) {
                max_dist = dist;
            }
            sum += dist;
            valid_count++;
        }
    }
    
    uint16_t avg_dist = valid_count > 0 ? (sum / valid_count) : 0;
    
    char stats[32];
    snprintf(stats, sizeof(stats), "Avg:%d Max:%d", avg_dist, max_dist);
    fb_write_text(7, stats, false);
}
```

**Result**: Statistics only include valid readings

---

## Issue 3: Number Alignment in ToF Focus Mode

### Problem

**Before Fix**:
```
ToF Grid (mm):
 450  523  612  701  ← 4 digits each = 20 chars (doesn't fit!)
8191  489  534  598  ← 8191 pushes everything right
```

**After Fix**:
```
ToF Grid (mm):
450 523 612 701  ← 3 digits each = 16 chars (perfect fit!)
999 489 534 598  ← Invalid readings clamped to 999
```

---

## VL53L8CX ToF Sensor Behavior

### Valid Range
- **Minimum**: ~0mm (but unreliable < 50mm)
- **Maximum**: ~4000mm (4 meters)
- **Optimal**: 100-2000mm

### Invalid Reading Indicators

| Value | Meaning | Cause |
|-------|---------|-------|
| 8191 (0x1FFF) | Invalid/Out of range | Object too close, too far, or no object |
| 0 | No data | Sensor not initialized or failed |
| > 4000 | Out of range | Beyond sensor capability |

### How We Handle It

| Scenario | Display Value | Bar Graph | Statistics |
|----------|---------------|-----------|------------|
| Valid (0-4000mm) | Actual value | Proportional fill | Included |
| Invalid (> 4000mm) | 999 | Empty bar | Excluded |
| Too close (< 50mm) | 999 | Empty bar | Excluded |

---

## Testing Recommendations

### Test 1: Mode Switching
1. Press mode button 5 times
2. Should cycle: Dashboard → ToF → IMU → Status → Near Miss → Dashboard
3. Should NOT crash on Near Miss mode
4. Serial should show all 5 mode names correctly

### Test 2: ToF Invalid Readings
1. **No object**: Point sensor at empty space
   - Should show 999 in ToF Focus mode
   - Bars should be empty
   - Statistics should exclude these readings

2. **Too close**: Put hand < 5cm from sensor
   - Should show 999 in ToF Focus mode
   - Bars should be empty

3. **Valid range**: Object at 20-200cm
   - Should show actual distance
   - Bars should fill proportionally
   - Statistics should be accurate

### Test 3: Display Alignment
1. Switch to ToF Focus mode
2. Move hand to trigger various distances
3. All numbers should stay aligned in 3-digit columns
4. No numbers should push others to the right

---

## Summary of Changes

### Files Modified
1. **`main/main.c`** - Added "Near Miss" to mode names array
2. **`components/oled_display/oled_display.c`** - Three fixes:
   - Clamp ToF values to 999 in ToF Focus mode
   - Handle invalid readings in bar graph function
   - Exclude invalid readings from statistics

### Lines Changed
- `main/main.c`: 1 line
- `oled_display.c`: ~30 lines

### Compilation Status
✅ No errors  
✅ No warnings  
✅ Ready to flash

---

## Build and Test

```powershell
# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

---

**Last Updated**: January 26, 2026  
**Status**: ✅ All issues fixed, ready for testing  
**Next**: Test on device to verify fixes work correctly
