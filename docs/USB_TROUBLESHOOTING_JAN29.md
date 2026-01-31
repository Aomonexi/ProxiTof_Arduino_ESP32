# USB Transfer Mode Troubleshooting - January 29, 2026

## Current Issues

1. ❌ ACTION button doesn't exit USB Transfer mode (stuck in Mode 7)
2. ❌ Mode change button disconnects/reconnects USB
3. ❌ PC doesn't detect USB flash drive

---

## Diagnostic Steps

### Step 1: Verify Console Configuration

Check your `sdkconfig` file:

```bash
# Search for these lines
grep "CONFIG_ESP_CONSOLE" sdkconfig
```

**Expected output**:
```
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set
# CONFIG_ESP_CONSOLE_USB_CDC is not set
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
# CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG is not set
```

If you see `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`, the config didn't save correctly.

---

### Step 2: Check Serial Monitor Output

Connect to **RIGHT port** and run:
```bash
idf.py monitor
```

**Navigate to Mode 7 (USB Transfer) and press ACTION button.**

Look for these messages:
```
I (xxxxx) SENSOR_MGR: Button pressed (held for XXX ms)
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
```

**If you DON'T see these messages**, the button handler isn't working.

---

### Step 3: Check TinyUSB Initialization

Look for these messages at startup:
```
I (5684) TinyUSB: TinyUSB Driver installed on port 0
I (5734) USB_MSC_SD: ✓ MSC driver installed
I (5754) USB_MSC_SD: ✓ USB MSC initialized successfully
```

**If you see errors**, TinyUSB isn't initializing correctly.

---

### Step 4: Check USB Descriptors

Look for these messages at startup:
```
W (5494) tusb_desc: No Device descriptor provided, using default.
W (5504) tusb_desc: No Full-speed configuration descriptor provided, using default.
W (5514) tusb_desc: No String descriptors provided, using default.
```

These warnings are **NORMAL** - default descriptors should work.

---

### Step 5: Test USB Detection

**In Mode 7, after pressing ACTION:**

1. Check serial monitor for:
```
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) USB_MSC_SD: 🔌 Connect LEFT port (USB-OTG) to computer
```

2. Connect LEFT port to PC

3. Look for:
```
I (xxxxx) USB_MSC_SD: 🔌 USB host connected
```

**If you DON'T see "USB host connected"**, the PC isn't detecting the device at all.

---

## Common Problems and Solutions

### Problem 1: Button Handler Not Working

**Symptoms**: Pressing ACTION button does nothing, no messages in serial monitor

**Possible causes**:
1. Button GPIO conflict
2. OLED display handler not being called
3. Button debounce issue

**Debug**:
```c
// Check if button is being detected at all
// Look for this in main.c around line 930:
if (current_mode == 7 && oled_available) {  // USB Transfer mode
    if (sensor_manager_is_button_pressed()) {
        ESP_LOGI(TAG, "💾 USB Transfer mode toggle triggered from button");
```

**Solution**: Check if `sensor_manager_is_button_pressed()` is returning true.

---

### Problem 2: USB Disconnects When Changing Modes

**Symptoms**: PC disconnects/reconnects when pressing mode change button

**Cause**: Mode change button (GPIO 20) cycles through ALL modes, including entering/exiting USB Transfer mode.

**Solution**: 
- **Stay in Mode 7** (USB Transfer)
- Use **ACTION button** (GPIO 3) to toggle USB on/off
- Don't press mode change button while in USB mode

---

### Problem 3: PC Doesn't Detect USB Device

**Symptoms**: No new drive appears, Device Manager shows nothing

**Possible causes**:

#### A. USB Cable Issue
- **Test**: Try a different USB cable (must support data, not just power)
- **Test**: Try a different PC USB port

#### B. TinyUSB Not Running
- **Check**: Look for TinyUSB task in logs
- **Check**: Verify `tusb_inited()` returns true

#### C. USB Descriptors Wrong
- **Check**: Look for descriptor warnings at startup
- **Check**: Verify VID/PID are correct (0x303a/0x4002)

#### D. Hardware Issue
- **Test**: Try using LEFT port for programming (if it works, hardware is OK)
- **Test**: Check if LEFT port provides power (LED should light up)

---

## Detailed Debugging Commands

### Check if TinyUSB is Initialized

Add this to your code temporarily:
```c
// In main loop, print TinyUSB status
static uint32_t last_tusb_debug = 0;
if ((current_time - last_tusb_debug) >= 5000) {
    last_tusb_debug = current_time;
    ESP_LOGI(TAG, "TinyUSB initialized: %s", tusb_inited() ? "YES" : "NO");
    ESP_LOGI(TAG, "TinyUSB mounted: %s", tud_mounted() ? "YES" : "NO");
}
```

### Check Button State

Add this to see if button is working:
```c
// In main loop, check button state
if (current_mode == 7) {  // USB Transfer mode
    bool button_state = sensor_manager_is_button_pressed();
    static bool last_button_state = false;
    
    if (button_state != last_button_state) {
        ESP_LOGI(TAG, "Button state changed: %s", button_state ? "PRESSED" : "RELEASED");
        last_button_state = button_state;
    }
}
```

---

## What to Check Next

Please provide the following information:

### 1. Serial Monitor Output
```
# When you press ACTION button in Mode 7, what do you see?
# Copy and paste the logs here
```

### 2. Startup Logs
```
# Copy the TinyUSB initialization section
# Look for lines with "TinyUSB", "USB_MSC_SD", "tusb_desc"
```

### 3. sdkconfig Verification
```bash
# Run this command and share the output:
grep "CONFIG_ESP_CONSOLE\|CONFIG_TINYUSB" sdkconfig
```

### 4. PC Behavior
- **Windows**: Check Device Manager → Universal Serial Bus devices
- **Linux**: Run `lsusb` before and after connecting
- **macOS**: Check System Information → USB

---

## Quick Test: Bypass OLED Handler

To test if the problem is in the OLED handler, try calling the USB functions directly:

Add this temporary code in `main.c`:
```c
// In main loop, add a manual test
static bool manual_test_done = false;
if (!manual_test_done && (current_time > 10000)) {  // After 10 seconds
    manual_test_done = true;
    
    ESP_LOGI(TAG, "=== MANUAL USB TEST ===");
    ESP_LOGI(TAG, "Activating USB Transfer mode...");
    
    esp_err_t ret = usb_msc_sdcard_pause_logging();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Transfer mode activated");
        ESP_LOGI(TAG, "Connect LEFT port to PC now");
        ESP_LOGI(TAG, "Check if PC detects drive");
    } else {
        ESP_LOGE(TAG, "❌ Failed: %s", esp_err_to_name(ret));
    }
}
```

This will automatically activate USB Transfer mode 10 seconds after startup, bypassing the button handler.

---

## Expected Working Behavior

### Correct Button Usage

**Mode Change Button (GPIO 20)**:
- Cycles through modes: 0 → 1 → 2 → 3 → 4 → 5 → 6 → **7** → 8 → 0
- Use to **navigate TO** Mode 7
- **Don't press while in Mode 7** (will exit to Mode 8)

**ACTION Button (GPIO 3)**:
- **Only works when already in Mode 7**
- First press: Activate USB Transfer
- Second press: Deactivate USB Transfer
- Toggles between logging and USB modes

### Correct Sequence

```
1. Press Mode Change button until you reach Mode 7 (USB Transfer)
   OLED shows: "USB Transfer" header

2. Press ACTION button (GPIO 3)
   OLED shows: "Activating..."
   Then: "[USB] Connect LEFT port" + "ACTION: Cancel"

3. Connect LEFT port to PC
   PC should detect USB flash drive
   Serial monitor shows: "USB host connected"

4. Access files on PC

5. Disconnect USB cable from PC

6. Press ACTION button again (GPIO 3)
   OLED shows: "Deactivating..."
   Then: "Logging resumed"

7. System returns to normal logging
```

---

## Next Steps

1. Share the serial monitor output when pressing ACTION in Mode 7
2. Share the startup logs (TinyUSB section)
3. Verify sdkconfig settings
4. Try the manual test code above

This will help us identify exactly where the problem is!
