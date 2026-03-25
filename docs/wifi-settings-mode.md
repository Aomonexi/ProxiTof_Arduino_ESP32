# WiFi Settings Mode - User Guide

**Date:** January 26, 2026  
**Feature:** New display mode for WiFi control and time synchronization  
**Status:** ✅ Implemented

---

## Overview

The WiFi Settings mode provides an easy way to check time synchronization status and manually trigger WiFi sync without connecting to USB or editing config files.

---

## Button Layout

### Button 1 (GPIO 20) - Mode/Navigation
**Function:** Cycle through display modes

**Modes:**
1. Dashboard
2. ToF Focus
3. IMU Focus
4. Status
5. Near Miss
6. **WiFi Settings** ← New!

**Usage:** Press to cycle through modes

---

### Button 2 (GPIO 3) - Action (Context-Aware)

**Function depends on current mode:**

| Mode | Button 2 Action |
|------|----------------|
| **Dashboard** | Flag event (manual near-miss logging) |
| **ToF Focus** | No action |
| **IMU Focus** | No action |
| **Status** | No action |
| **Near Miss** | Flag event (manual near-miss logging) |
| **WiFi Settings** | **Trigger WiFi sync** ← New! |

---

## WiFi Settings Mode Layout

### Display Layout

```
┌────────────────────────────────┐
│ WiFi Settings        (INVERTED)│ ← Yellow zone (header)
│ Status: SYNCED                 │
├────────────────────────────────┤
│ Last: 2h ago                   │ ← Blue zone (info)
│ Syncs: 5                       │
│ Source: DS3231 RTC             │
│                                │
│ Press ACTION to                │
│ sync time (WiFi)               │
└────────────────────────────────┘
```

### Information Displayed

**Line 1 (Inverted):** "WiFi Settings"
- Inverted header for easy identification

**Line 2:** Sync Status
- "Status: SYNCED" - Time has been synchronized
- "Status: NOT SYNCED" - Time never synchronized

**Line 3:** Last Sync Time
- "Last: <1h ago" - Synced less than 1 hour ago
- "Last: 5h ago" - Synced 5 hours ago
- "Last: 2d ago" - Synced 2 days ago
- "Last: Never" - Never synchronized

**Line 4:** Sync Count
- "Syncs: 5" - Number of successful syncs since boot

**Line 5:** Time Source
- "Source: DS3231 RTC" - Using DS3231 (best)
- "Source: Internal RTC" - Using ESP32 internal RTC
- "Source: NTP" - Just synced from internet
- "Source: None" - No valid time

**Lines 7-8:** Instructions
- "Press ACTION to"
- "sync time (WiFi)"

---

## Usage Scenarios

### Scenario 1: Check Time Sync Status

**Steps:**
1. Press Button 1 (Mode) until you reach "WiFi Settings"
2. Read the display:
   - Check sync status
   - Check last sync time
   - Check time source

**Example:**
```
WiFi Settings
Status: SYNCED
Last: 12h ago
Syncs: 3
Source: DS3231 RTC
```

**Interpretation:** Time is synced, last updated 12 hours ago, using DS3231 RTC (good!)

---

### Scenario 2: Manual WiFi Sync

**When to use:**
- Time seems incorrect
- Haven't synced in a long time
- Just changed timezone
- Want to ensure accurate time before field deployment

**Steps:**
1. Press Button 1 (Mode) until you reach "WiFi Settings"
2. Press Button 2 (Action) to trigger sync
3. Display shows "Connecting..."
4. Wait for result:
   - "Sync SUCCESS!" - Time updated
   - "Sync FAILED" - Check WiFi config

**Example:**
```
WiFi Settings
Connecting...
Please wait

↓ (after 5-10 seconds)

WiFi Settings
Sync SUCCESS!
Time updated

↓ (returns to WiFi Settings after 2 seconds)

WiFi Settings
Status: SYNCED
Last: <1h ago
Syncs: 4
Source: NTP
```

---

### Scenario 3: Field Deployment Check

**Before going to field:**
1. Navigate to WiFi Settings mode
2. Check sync status
3. If "Last: >24h ago" or "Status: NOT SYNCED":
   - Press Button 2 to sync
   - Wait for "Sync SUCCESS!"
4. Verify time source is "DS3231 RTC"
5. Ready for field use!

---

## WiFi Sync Process

### What Happens When You Press Button 2:

```
1. Display shows "Connecting..."
   ↓
2. ESP32 connects to WiFi (from config.txt)
   ↓
3. Contacts NTP server (pool.ntp.org)
   ↓
4. Gets current time
   ↓
5. Updates ESP32 internal RTC
   ↓
6. Updates DS3231 RTC (battery-backed)
   ↓
7. Disconnects WiFi (saves power)
   ↓
8. Display shows "Sync SUCCESS!"
   ↓
9. Returns to WiFi Settings (after 2 seconds)
```

**Duration:** 5-10 seconds (depending on WiFi signal)

**Power:** WiFi active only during sync, then disconnected

---

## Troubleshooting

### Problem: "Sync FAILED" Message

**Possible Causes:**
1. WiFi credentials wrong
2. Out of WiFi range
3. Router is 5GHz only (ESP32 needs 2.4GHz)
4. Internet connection down

**Solutions:**
1. Check `data/config.txt`:
   ```ini
   WIFI_ENABLED=true
   WIFI_SSID=YourNetwork
   WIFI_PASSWORD=YourPassword
   ```
2. Move closer to router
3. Check router is broadcasting 2.4GHz
4. Check internet connection

---

### Problem: Time Still Wrong After Sync

**Possible Causes:**
1. Timezone setting incorrect
2. DS3231 battery dead

**Solutions:**
1. Check timezone in `data/config.txt`:
   ```ini
   # For UTC+8 (China/Singapore/Philippines):
   TIMEZONE=<+08>-8
   
   # For UTC+7 (Indonesia/Thailand):
   TIMEZONE=WIB-7
   
   # For UTC+9 (Japan):
   TIMEZONE=JST-9
   ```
2. Replace DS3231 CR2032 battery

---

### Problem: Button 2 Does Nothing

**Possible Causes:**
1. Not in WiFi Settings mode
2. Button debounce delay
3. WiFi disabled in config

**Solutions:**
1. Press Button 1 until you see "WiFi Settings" header
2. Wait 200ms between button presses
3. Check `WIFI_ENABLED=true` in config.txt

---

## Configuration

### Enable WiFi Sync

**File:** `data/config.txt`

```ini
# WiFi Configuration
WIFI_ENABLED=true
WIFI_SSID=YourNetwork
WIFI_PASSWORD=YourPassword

# Time Configuration
TIMEZONE=<+08>-8  # Change to your timezone
SYNC_INTERVAL_HOURS=24  # Auto-sync every 24 hours
```

### Disable WiFi (Field Use)

```ini
# WiFi Configuration
WIFI_ENABLED=false  # Disable WiFi completely

# Time Configuration
TIMEZONE=<+08>-8
SYNC_INTERVAL_HOURS=0  # Disable auto-sync
```

**Note:** With WiFi disabled, Button 2 in WiFi Settings mode will show "WiFi disabled" message.

---

## Power Consumption

### WiFi Sync Impact

**During Sync (5-10 seconds):**
- ESP32: ~160mA (WiFi active)
- Duration: 5-10 seconds
- Energy: ~0.4-0.8 mAh

**After Sync (normal operation):**
- ESP32: ~40mA (WiFi off)
- DS3231: ~200µA
- Total: ~40mA

**Impact:** Negligible! One sync uses <0.1% of 2000mAh battery.

---

## Comparison: Manual vs Automatic Sync

### Manual Sync (WiFi Settings Mode)

**Pros:**
- ✅ User control
- ✅ Sync only when needed
- ✅ Visual feedback
- ✅ No config changes needed

**Cons:**
- ❌ Requires user action
- ❌ Easy to forget

**Best for:** Field deployment, testing, troubleshooting

---

### Automatic Sync (Config File)

**Pros:**
- ✅ No user action needed
- ✅ Always up-to-date
- ✅ Set and forget

**Cons:**
- ❌ Requires WiFi always available
- ❌ Slightly higher power consumption
- ❌ May sync when not needed

**Best for:** Lab use, long-term deployment with WiFi

---

## Advanced Features (Future)

### Planned Enhancements:

1. **WiFi Status Indicator**
   - Show WiFi signal strength
   - Show connection status

2. **Manual WiFi Toggle**
   - Enable/disable WiFi from display
   - No need to edit config file

3. **Sync Schedule**
   - Configure sync interval from display
   - "Sync now", "Sync daily", "Sync weekly"

4. **Network Selection**
   - Scan for available networks
   - Select network from display

---

## Summary

### What You Get:

- ✅ Easy time sync status check
- ✅ One-button WiFi sync
- ✅ Visual feedback
- ✅ Context-aware button behavior
- ✅ No USB connection needed
- ✅ No config file editing needed

### Button Behavior:

| Mode | Button 1 | Button 2 |
|------|----------|----------|
| Dashboard | Next mode | Flag event |
| ToF Focus | Next mode | - |
| IMU Focus | Next mode | - |
| Status | Next mode | - |
| Near Miss | Next mode | Flag event |
| **WiFi Settings** | Next mode | **WiFi sync** |

### Quick Reference:

**Check sync status:** Button 1 → WiFi Settings  
**Trigger sync:** Button 1 → WiFi Settings → Button 2  
**Flag event:** Button 1 → Dashboard → Button 2

---

**Status:** ✅ Implemented and ready for testing  
**Documentation:** ✅ Complete  
**User-friendly:** ✅ Yes!

