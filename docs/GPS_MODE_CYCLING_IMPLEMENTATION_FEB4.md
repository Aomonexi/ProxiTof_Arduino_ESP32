# GPS Mode Cycling Implementation - Complete

**Date**: February 4, 2026  
**Status**: ✅ COMPLETE  
**Feature**: Hold-to-cycle GPS dynamic model selection

---

## Summary

Implemented dual-function ACTION button in GPS mode:
- **Quick press (< 2 seconds)**: GPS Reconnect (refresh)
- **Hold (≥ 2 seconds)**: Enter GPS mode cycling
- **In cycling mode**:
  - Tap ACTION → Cycle through models
  - Press MODE → Save and exit

---

## Implementation Details

### 1. State Variables (oled_display.c)

```c
// GPS mode cycling state
static bool gps_mode_cycling_active = false;
static uint8_t gps_mode_cycling_model = 0;  // Current model being cycled
static uint32_t gps_mode_cycling_start_time = 0;
```

### 2. Helper Function (oled_display.c)

```c
/**
 * Cycle to next valid GPS dynamic model
 * Valid models: 0=Portable, 3=Pedestrian, 4=Automotive, 10=Bike
 */
static uint8_t cycle_gps_model(uint8_t current_model) {
    switch (current_model) {
        case 0:  return 3;   // Portable → Pedestrian
        case 3:  return 4;   // Pedestrian → Automotive
        case 4:  return 10;  // Automotive → Bike
        case 10: return 0;   // Bike → Portable
        default: return 10;  // Invalid → Bike (default)
    }
}
```

### 3. Public API Functions (oled_display.c)

```c
/**
 * Check if GPS mode cycling is active
 */
bool oled_display_is_gps_cycling_active(void);

/**
 * Cycle to next GPS dynamic model
 */
esp_err_t oled_display_cycle_gps_model(void);

/**
 * Save GPS model and exit cycling mode
 */
esp_err_t oled_display_save_gps_model(void);
```

### 4. Button Handler (oled_display.c)

Modified `oled_display_handle_action_button()` for GPS mode:

```c
case OLED_MODE_GPS:
    // Check if button is held (2+ seconds) for mode cycling
    // or quick press for GPS reconnect
    
    uint32_t hold_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    bool button_held = false;
    
    // Wait up to 2 seconds to see if button is held
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS - hold_start) < 2000) {
        if (!sensor_manager_is_button_held()) {
            break;  // Button released - quick press
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // Check if button is still held after 2 seconds
    if (sensor_manager_is_button_held() && 
        (xTaskGetTickCount() * portTICK_PERIOD_MS - hold_start) >= 2000) {
        button_held = true;
    }
    
    if (button_held) {
        // HOLD DETECTED - Enter GPS mode cycling
        gps_mode_cycling_active = true;
        gps_mode_cycling_model = cfg->gps_dynamic_model;
        // Show cycling UI...
    } else {
        // QUICK PRESS - GPS Reconnect
        // Reinitialize GPS...
    }
```

### 5. Display Update (oled_display.c)

Modified `oled_display_update()` for GPS mode:

```c
case OLED_MODE_GPS:
    // Check if we're in GPS mode cycling state
    if (gps_mode_cycling_active) {
        // Show GPS mode cycling UI
        clear_frame_buffer();
        fb_write_text(0, "GPS Mode Cycling", true);
        fb_write_text(2, "Current:", false);
        char model_line[32];
        snprintf(model_line, sizeof(model_line), " %s", 
                gps_get_model_name(gps_mode_cycling_model));
        fb_write_text(3, model_line, false);
        fb_write_text(5, "ACTION: Next", false);
        fb_write_text(6, "MODE: Save&Exit", false);
        fb_write_text(7, "Bike/Ped/Port/Auto", false);
        break;
    }
    
    // Normal GPS display (not in cycling mode)
    // ... existing GPS display code ...
```

### 6. Main Loop Integration (main.c)

Added button handling in main loop:

```c
} else if (current_mode == 4 && oled_available) {  // GPS mode
    // Check if we're in GPS mode cycling state
    if (oled_display_is_gps_cycling_active()) {
        // In cycling mode - handle ACTION and MODE buttons
        if (button_pressed) {
            // ACTION button: Cycle to next GPS model
            ESP_LOGI(TAG, "🔄 GPS model cycle triggered from button");
            
            // Cyan LED during cycling
            if (led_strip != NULL) {
                led_strip_set_pixel(led_strip, 0, 0, 255, 255);  // Cyan
                led_strip_refresh(led_strip);
            }
            
            oled_display_cycle_gps_model();
        }
        
        // Check MODE button to save and exit
        static bool last_mode_button_in_gps_cycling = true;
        bool current_mode_button = (gpio_get_level(MODE_BUTTON_PIN) == 0);
        
        // Detect button press (falling edge)
        if (current_mode_button && !last_mode_button_in_gps_cycling) {
            ESP_LOGI(TAG, "💾 GPS model save triggered from MODE button");
            
            // Green LED during save
            if (led_strip != NULL) {
                led_strip_set_pixel(led_strip, 0, 0, 255, 0);  // Green
                led_strip_refresh(led_strip);
            }
            
            oled_display_save_gps_model();
            
            ESP_LOGI(TAG, "✓ GPS model saved and cycling mode exited");
        }
        
        last_mode_button_in_gps_cycling = current_mode_button;
    } else {
        // Not in cycling mode - normal GPS reconnect behavior
        if (button_pressed) {
            ESP_LOGI(TAG, "📡 GPS action triggered from button");
            
            // Green LED during GPS action
            if (led_strip != NULL) {
                led_strip_set_pixel(led_strip, 0, 0, 255, 0);  // Green
                led_strip_refresh(led_strip);
            }
            
            // oled_display_handle_action_button() will detect hold vs quick press
            oled_display_handle_action_button();
        }
    }
}
```

---

## User Workflow

### Quick Press - GPS Reconnect

1. Navigate to GPS mode (Mode 4)
2. **Quick press** ACTION button (< 2 seconds)
3. Display shows "GPS Reconnect" → "Reinitializing..."
4. GPS module restarts
5. Display shows "SUCCESS!" or "FAILED"
6. Returns to normal GPS display

### Hold - GPS Mode Cycling

1. Navigate to GPS mode (Mode 4)
2. **Hold** ACTION button (≥ 2 seconds)
3. Display shows "GPS Mode Cycling" screen
4. Current model is displayed (e.g., "Bike")
5. **Press ACTION** to cycle: Bike → Pedestrian → Portable → Automotive → Bike
6. **Press MODE** button to save and exit
7. Model is saved to config file
8. Model is applied to GPS module
9. Returns to normal GPS display

---

## LED Indicators

| Action | LED Color | Meaning |
|--------|-----------|---------|
| Quick press ACTION | Green | GPS reconnecting |
| Hold ACTION 2+ sec | Cyan | Entering cycling mode |
| ACTION in cycling | Cyan | Cycling models |
| MODE in cycling | Green | Saving model |

---

## Available GPS Models

| Model | Value | Description | Use Case |
|-------|-------|-------------|----------|
| Bike | 10 | Optimized for cycling | Default, best for thesis |
| Pedestrian | 3 | Optimized for walking | Walking data collection |
| Portable | 0 | General purpose | General use |
| Automotive | 4 | Optimized for cars | Comparison data |

---

## Configuration Persistence

**Config File**: `/sdcard/config.txt`

**Setting**: `GPS_DYNAMIC_MODEL=10`

**Values**:
- `0` = Portable
- `3` = Pedestrian
- `4` = Automotive
- `10` = Bike (default)

**Behavior**:
- Setting is saved when MODE button is pressed in cycling mode
- Loads automatically on boot
- Applied to GPS module immediately
- Persists across power cycles

---

## Files Modified

### 1. components/oled_display/oled_display.c
- Added GPS mode cycling state variables
- Added `cycle_gps_model()` helper function
- Implemented `oled_display_is_gps_cycling_active()`
- Implemented `oled_display_cycle_gps_model()`
- Implemented `oled_display_save_gps_model()`
- Modified `oled_display_handle_action_button()` for GPS mode
- Modified `oled_display_update()` to show cycling UI

### 2. components/oled_display/include/oled_display.h
- Added function declarations:
  - `bool oled_display_is_gps_cycling_active(void)`
  - `esp_err_t oled_display_cycle_gps_model(void)`
  - `esp_err_t oled_display_save_gps_model(void)`

### 3. main/main.c
- Modified GPS mode button handling in main loop
- Added check for `oled_display_is_gps_cycling_active()`
- Added ACTION button handler for cycling
- Added MODE button handler for saving

### 4. docs/Current Modes and Layouts.md
- Updated GPS mode section with cycling feature
- Added cycling mode display diagram
- Added button behavior documentation
- Added LED indicators table
- Updated testing checklist

---

## Testing Checklist

- [x] Build succeeds without errors
- [ ] GPS mode appears in cycle (Mode 4)
- [ ] Quick press ACTION triggers GPS reconnect
- [ ] Hold ACTION 2+ seconds enters mode cycling
- [ ] Cycling mode shows current model name
- [ ] ACTION in cycling mode cycles through models
- [ ] MODE in cycling mode saves and exits
- [ ] Model persists after power cycle
- [ ] Config file shows GPS_DYNAMIC_MODEL=X
- [ ] GPS module applies new model immediately
- [ ] LED indicators work correctly
- [ ] No crashes or freezes

---

## Benefits

1. **Field Configuration** - Change GPS model without editing config file
2. **Instant Feedback** - See model name on display immediately
3. **Easy Testing** - Cycle through all models to find best performance
4. **Persistent Settings** - Model saved to config file automatically
5. **Dual Function** - Quick press for reconnect, hold for cycling
6. **User Friendly** - Clear instructions on display
7. **Safe Operation** - MODE button required to save (prevents accidental changes)

---

## Next Steps

1. Flash firmware to device
2. Test quick press (GPS reconnect)
3. Test hold (enter cycling mode)
4. Test ACTION in cycling (cycle models)
5. Test MODE in cycling (save and exit)
6. Verify config file persistence
7. Test GPS behavior with different models
8. Document field test results

---

## Related Documentation

- `docs/GPS_DYNAMIC_MODEL_AND_ANTENNA_GUIDE.md` - GPS antenna placement
- `docs/GPS_DYNAMIC_MODEL_IMPLEMENTATION_FEB4.md` - Config file implementation
- `docs/Current Modes and Layouts.md` - Complete OLED mode reference
- `docs/BUTTON_COMPLETE_GUIDE.md` - Button behavior reference
- `components/gps/gps.c` - GPS module implementation
- `components/config_manager/config_manager.c` - Config system

---

## Conclusion

GPS mode cycling feature is now fully implemented and ready for testing. The dual-function button design provides both quick GPS reconnect and model cycling in a single, intuitive interface. The feature integrates seamlessly with the existing config system and provides immediate visual feedback on the OLED display.

**Status**: ✅ COMPLETE - Ready for field testing
