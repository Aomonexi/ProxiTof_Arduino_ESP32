# Configuration File Guide - Where to Change WiFi Settings

**Date:** January 26, 2026  
**Question:** Where do I change WiFi settings?  
**Answer:** It depends on your use case!

---

## Quick Answer

### 🎯 For Normal Use: Change `config.txt` on SD Card

**Location:** SD card root → `config.txt`

**Why:** This is the **ONLY** file the device actually reads!

---

## The Three Config Files Explained

### 1. `data/config.txt` (Template - Not Used Directly)

**Location:** `data/config.txt` (in your project folder)

**Purpose:** 
- Template/example file
- Shows you what settings are available
- Used to create initial SD card config

**Is it used by the device?** ❌ **NO!**

**When to edit:**
- When you want to update the template
- Before first SD card setup
- To remember what settings exist

---

### 2. `data/wifi_config.txt` (Old File - Deprecated)

**Location:** `data/wifi_config.txt` (in your project folder)

**Purpose:** 
- Old WiFi-only config file
- **No longer used!**
- Kept for reference only

**Is it used by the device?** ❌ **NO!**

**Should you edit it?** ❌ **NO!** (It's obsolete)

---

### 3. `/sdcard/config.txt` (THE REAL ONE!)

**Location:** SD card root → `config.txt`

**Purpose:**
- **This is the ONLY file the device reads!**
- Contains all system settings
- Can be edited without recompiling code

**Is it used by the device?** ✅ **YES!**

**When to edit:**
- To change WiFi credentials
- To change timezone
- To adjust any system setting

---

## How It Works

### Startup Process

```
1. Device powers on
   ↓
2. Loads default settings from code
   (config_load_defaults in config_manager.c)
   ↓
3. Tries to read /sdcard/config.txt
   ↓
4. If found: Overrides defaults with SD card settings ✅
   If not found: Uses defaults from code ⚠️
   ↓
5. System starts with final configuration
```

### Code Flow

```c
// In main.c
void app_main(void) {
    system_config_t config;
    
    // 1. Load defaults (hardcoded in config_manager.c)
    config_load_defaults(&config);
    
    // 2. Try to load from SD card
    esp_err_t err = config_load_from_sd(&config);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Using config from SD card");
    } else {
        ESP_LOGW(TAG, "⚠️  Using default config (no SD card file)");
    }
    
    // 3. Use the config
    smart_time_init(..., config.wifi_ssid, config.wifi_password, ...);
}
```

---

## How to Change WiFi Settings

### Method 1: Edit SD Card File (Recommended)

**Steps:**

1. **Remove SD card** from ESP32
2. **Insert into computer**
3. **Open** `config.txt` in text editor
4. **Find WiFi section:**
   ```ini
   # WiFi Configuration
   WIFI_ENABLED=true
   WIFI_SSID=YourWiFiNetwork
   WIFI_PASSWORD=YourWiFiPassword
   ```
5. **Change** to your WiFi:
   ```ini
   # WiFi Configuration
   WIFI_ENABLED=true
   WIFI_SSID=MyHomeWiFi
   WIFI_PASSWORD=MySecurePassword123
   ```
6. **Save** the file
7. **Eject** SD card safely
8. **Insert back** into ESP32
9. **Power on** device

**Result:** Device uses your new WiFi settings!

---

### Method 2: Create SD Card Config (First Time)

**If SD card doesn't have config.txt yet:**

1. **Copy** `data/config.txt` from project folder
2. **Paste** to SD card root
3. **Rename** if needed (should be `config.txt`)
4. **Edit** WiFi settings
5. **Save** and insert into ESP32

---

### Method 3: Change Code Defaults (Not Recommended)

**Only if you don't have SD card:**

**File:** `components/config_manager/config_manager.c`

**Find this function:**
```c
void config_load_defaults(system_config_t *config) {
    // Default configuration
    strcpy(config->wifi_ssid, "YourWiFiNetwork");
    strcpy(config->wifi_password, "YourWiFiPassword");
    strcpy(config->timezone, "CST-8");
    // ...
}
```

**Change to:**
```c
void config_load_defaults(system_config_t *config) {
    // Default configuration
    strcpy(config->wifi_ssid, "MyHomeWiFi");
    strcpy(config->wifi_password, "MyPassword123");
    strcpy(config->timezone, "<+08>-8");
    // ...
}
```

**Then rebuild and flash:**
```bash
idf.py build flash
```

**Why not recommended:**
- Requires recompiling
- WiFi password in code (security risk)
- Can't change without USB connection

---

## File Priority

```
┌─────────────────────────────────────────┐
│  Priority 1: /sdcard/config.txt         │
│  (If exists, overrides everything)      │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  Priority 2: Code defaults              │
│  (Used if no SD card file)              │
└─────────────────────────────────────────┘
```

**Simple rule:** SD card config **always wins** if it exists!

---

## Common Scenarios

### Scenario 1: Lab Testing (WiFi Available)

**SD Card config.txt:**
```ini
WIFI_ENABLED=true
WIFI_SSID=LabNetwork
WIFI_PASSWORD=LabPassword123
TIMEZONE=<+08>-8
SYNC_INTERVAL_HOURS=1  # Sync every hour
```

**Result:** Device syncs time every hour via WiFi

---

### Scenario 2: Field Deployment (No WiFi)

**Before deployment:**
1. Enable WiFi temporarily
2. Let device sync time once
3. Edit SD card config:
   ```ini
   WIFI_ENABLED=false
   TIMEZONE=<+08>-8
   SYNC_INTERVAL_HOURS=0  # Disable auto-sync
   ```
4. Reinsert SD card

**Result:** Device uses DS3231 RTC, no WiFi needed

---

### Scenario 3: Multiple Devices

**Device 1 SD card:**
```ini
WIFI_SSID=HomeNetwork
WIFI_PASSWORD=HomePass
```

**Device 2 SD card:**
```ini
WIFI_SSID=OfficeNetwork
WIFI_PASSWORD=OfficePass
```

**Result:** Each device uses its own WiFi!

---

## Troubleshooting

### Problem: Device not connecting to WiFi

**Check:**
1. ✅ SD card is inserted
2. ✅ File is named `config.txt` (not `config.txt.txt`)
3. ✅ File is in SD card root (not in a folder)
4. ✅ WIFI_ENABLED=true
5. ✅ SSID and password are correct
6. ✅ No extra spaces in values
7. ✅ WiFi is 2.4GHz (ESP32 doesn't support 5GHz)

**Console output to look for:**
```
✓ Configuration loaded from SD card
Setting timezone: <+08>-8
Connecting to WiFi SSID: MyHomeWiFi
✓ Connected to WiFi
```

---

### Problem: Device uses wrong WiFi

**Cause:** SD card config.txt has old settings

**Solution:**
1. Remove SD card
2. Edit config.txt on computer
3. Save changes
4. Reinsert SD card
5. Restart device

---

### Problem: Can't find config.txt on SD card

**Solution:**
1. Copy `data/config.txt` from project folder
2. Paste to SD card root
3. Edit WiFi settings
4. Save and insert

---

## Best Practices

### ✅ DO:

- Keep SD card config.txt updated
- Use SD card for WiFi settings (easy to change)
- Test WiFi connection before field deployment
- Keep a backup of config.txt on computer

### ❌ DON'T:

- Don't edit `data/wifi_config.txt` (it's not used!)
- Don't hardcode WiFi password in code
- Don't forget to save config.txt after editing
- Don't use 5GHz WiFi (ESP32 only supports 2.4GHz)

---

## Quick Reference

### Where to Change WiFi Settings

| File | Location | Used by Device? | When to Edit |
|------|----------|----------------|--------------|
| **`/sdcard/config.txt`** | **SD card root** | **✅ YES** | **Always edit this!** |
| `data/config.txt` | Project folder | ❌ No (template) | To update template |
| `data/wifi_config.txt` | Project folder | ❌ No (obsolete) | Never (deprecated) |

### File Locations

```
Your Computer:
  ProxiTof_Arduino_ESP32/
  ├── data/
  │   ├── config.txt          ← Template (not used by device)
  │   └── wifi_config.txt     ← Old file (not used)
  └── components/
      └── config_manager/
          └── config_manager.c ← Code defaults

SD Card:
  /
  └── config.txt              ← THE REAL ONE! Edit this!
```

---

## Summary

**Simple Answer:**

> **Change WiFi settings in `config.txt` on the SD card root.**
> 
> That's the ONLY file the device reads!

**Steps:**
1. Remove SD card
2. Edit `config.txt` on computer
3. Change WIFI_SSID and WIFI_PASSWORD
4. Save file
5. Insert SD card back
6. Restart device

**Files to ignore:**
- `data/config.txt` (just a template)
- `data/wifi_config.txt` (obsolete, not used)

---

**Status:** ✅ Clear and simple!  
**Remember:** SD card config.txt is the boss! 👑

