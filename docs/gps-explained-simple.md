# GPS Explained Simply - Like Google Maps for Your Bike

**Date**: February 3, 2026 (Updated: February 5, 2026)  
**Purpose**: Simple explanation of how GPS works in ProxiTof  
**Audience**: Non-technical users

---

## What is GNSS? (More Than Just GPS!)

**GNSS** = Global Navigation Satellite System

ProxiTof uses **6 different satellite systems** for better accuracy:

1. 🇺🇸 **GPS** (USA) - 8 channels
2. 🇷🇺 **GLONASS** (Russia) - 8 channels
3. 🇪🇺 **Galileo** (Europe) - 4 channels
4. 🇨🇳 **BeiDou** (China) - 8 channels
5. 🇯🇵 **QZSS** (Japan) - 2 channels
6. 🌍 **SBAS** (Augmentation) - 1 channel

**Total: 31 satellite tracking channels!**

### Why Multiple Systems?

**More satellites = Better accuracy!**

```
Using only GPS:          Using all 6 systems:
   🛰️  🛰️                  🛰️ 🛰️ 🛰️ 🛰️
      🛰️                  🛰️ 🛰️ 🛰️ 🛰️
       🚴                    🛰️ 🚴 🛰️
   
   4-6 satellites         12-20 satellites
   Slower fix             Faster fix
   Less accurate          More accurate
```

**Benefits:**
- ⚡ Faster position fix (cold start: ~26s → potentially faster)
- 🎯 Better accuracy (more satellites = better geometry)
- 🏙️ Works better in cities (buildings block some satellites)
- 🌳 Works better under trees (more satellites visible)

---

## Q1: Will GPS keep updating my location automatically?

**YES! Just like Google Maps!** 📍

### **How It Works:**

```
Every 1 second (automatically):
┌─────────────────────────────────────┐
│ 1. GPS receives signals from        │
│    satellites in space (8-12 sats)  │
│                                     │
│ 2. Calculates your position         │
│    - Latitude (North/South)         │
│    - Longitude (East/West)          │
│    - Altitude (height above sea)    │
│    - Speed (how fast you're moving) │
│    - Heading (which direction)      │
│                                     │
│ 3. Updates display automatically    │
│    - No button press needed!        │
│    - Real-time like Google Maps     │
│                                     │
│ 4. Logs to SD card                  │
│    - Every position saved           │
│    - With timestamp                 │
│    - For later analysis             │
└─────────────────────────────────────┘
```

### **Example: Riding Your Bike**

```
Time    Location              Speed    Heading
────────────────────────────────────────────────
10:00   1.234567°N, 103.123°E  0 km/h   0° (stopped)
10:01   1.234580°N, 103.123°E  15 km/h  45° (moving NE)
10:02   1.234595°N, 103.124°E  18 km/h  90° (moving E)
10:03   1.234600°N, 103.125°E  12 km/h  135° (turning SE)
```

**Just like watching your blue dot move on Google Maps!**

### **What You See:**

**On OLED Display (GPS Mode):**
```
┌────────────────────────┐
│ GPS: 3D FIX (8sat)     │ ← Updates every second
│ HDOP: 1.2              │
├────────────────────────┤
│ Lat: 1.234567N         │ ← Changes as you move
│ Lon: 103.123456E       │ ← Changes as you move
│ Alt: 45.2m             │ ← Changes on hills
│ Spd: 15.3km/h          │ ← Updates with speed
│ Hdg: 90deg             │ ← Updates with direction
│ Age: 250ms             │ ← How fresh the data is
└────────────────────────┘
```

**In CSV Log File:**
```csv
timestamp,lat,lon,alt,speed,heading,satellites
10:00:00,1.234567,103.123456,45.2,0.0,0,8
10:00:01,1.234580,103.123456,45.3,15.3,45,8
10:00:02,1.234595,103.124000,45.5,18.2,90,9
```

---

## Q2: What is "Bike Mode" and why is it better?

**Bike Mode** tells the GPS: "I'm on a bicycle, optimize tracking for cycling!"

### **Simple Analogy:**

Think of GPS like a camera with different shooting modes:

- 📷 **Portrait Mode** = optimized for faces
- 🏃 **Sports Mode** = optimized for fast action
- 🌙 **Night Mode** = optimized for low light
- 🚴 **Bike Mode** = optimized for cycling!

### **What GPS Needs to Know:**

**Different vehicles move differently:**

| Vehicle | Speed | Turns | Stops | GPS Mode |
|---------|-------|-------|-------|----------|
| 🚶 Walking | 5 km/h | Slow, gentle | Frequent | Pedestrian |
| 🚴 **Cycling** | **15 km/h** | **Quick, sharp** | **Moderate** | **Bike** ✅ |
| 🚗 Driving | 60 km/h | Moderate | Rare | Automotive |
| ✈️ Flying | 500 km/h | Very slow | Never | Airborne |

### **Why Bike Mode Matters:**

**Scenario: You swerve to avoid a pothole**

**Portable Mode (default):**
```
GPS thinks: "Hmm, sudden turn? Must be GPS error, I'll smooth it out"
Your path:    ╱╲     (actual swerve)
GPS records:  ──     (smoothed, missed the swerve!)
Result: ❌ Doesn't capture evasive maneuver
```

**Bike Mode:**
```
GPS thinks: "Quick turn? Normal for cycling, I'll track it accurately"
Your path:    ╱╲     (actual swerve)
GPS records:  ╱╲     (accurate tracking!)
Result: ✅ Captures evasive maneuver correctly
```

### **Real-World Benefits:**

**1. Better Near-Miss Detection**
- Captures sudden swerves (avoiding cars)
- Records emergency braking (speed drops)
- Tracks evasive maneuvers accurately

**2. More Accurate Speed**
- Responds faster to speed changes
- Better for start/stop cycling
- Accurate during acceleration/braking

**3. Better Turn Tracking**
- Follows corners accurately
- Captures zigzag patterns
- Records actual path, not smoothed

**4. Optimized for Urban Cycling**
- Handles buildings (GPS multipath)
- Works better with trees
- Adapts to stop-and-go traffic

### **Technical Details (Optional):**

**Bike Mode Settings:**
- Speed range: 0-30 km/h (typical cycling)
- Acceleration: Medium (braking, starting)
- Turn rate: High (sharp corners, swerving)
- Altitude changes: Medium (hills, slopes)
- Position accuracy: 2-5 meters
- Update rate: 1 Hz (1 position/second)

**Kalman Filter Tuning:**
- More responsive to quick changes
- Less smoothing of position
- Better tracking of rapid movements
- Optimized for bicycle dynamics

---

## Q3: GPS Mode ACTION Button - What Does It Do?

**ACTION Button in GPS Mode = Force GPS Refresh** 🔄

### **When to Use:**

**Press ACTION button when:**
- ✅ GPS shows "SEARCHING" for too long (> 5 minutes)
- ✅ GPS lost fix and won't reacquire
- ✅ Satellite count stuck at 0
- ✅ Position seems frozen or wrong
- ✅ Just moved from indoors to outdoors

**What Happens:**
```
1. Press ACTION button (GPIO 3)
2. Display shows "GPS Reconnect"
3. GPS module resets
4. Starts fresh satellite search
5. Usually gets fix faster (30-60 seconds)
```

### **Example Scenario:**

**Problem:**
```
You were indoors (no GPS fix)
Then went outdoors
But GPS still shows "SEARCHING" after 5 minutes
Satellite count: 0
```

**Solution:**
```
1. Navigate to GPS mode (Mode 4)
2. Press ACTION button
3. Display shows "GPS Reconnect - Reinitializing..."
4. Wait 30-60 seconds
5. GPS finds satellites: "SUCCESS! Searching sats..."
6. Fix acquired in 1-2 minutes
```

### **What It Does Internally:**

```c
// When you press ACTION in GPS mode:
1. Stop GPS task
2. Close UART connection
3. Wait 200ms (GPS cleanup)
4. Reinitialize GPS with same settings
5. GPS starts fresh satellite search
6. Configuration still saved (bike mode, all GNSS)
```

**Like restarting Google Maps when it's stuck!**

---

## Q4: Is it like Google Maps? Easy to get location?

**YES! Even easier than Google Maps!** 📱

### **Comparison:**

| Feature | Google Maps | ProxiTof GPS |
|---------|-------------|--------------|
| **Automatic updates** | ✅ Yes | ✅ Yes |
| **Real-time position** | ✅ Yes | ✅ Yes |
| **Speed tracking** | ✅ Yes | ✅ Yes |
| **Direction/heading** | ✅ Yes | ✅ Yes |
| **Altitude** | ✅ Yes | ✅ Yes |
| **Satellite count** | ❌ No | ✅ Yes |
| **Accuracy (HDOP)** | ❌ No | ✅ Yes |
| **Data age** | ❌ No | ✅ Yes |
| **Offline logging** | ❌ No | ✅ Yes |
| **No internet needed** | ❌ Needs data | ✅ Works offline |
| **Battery usage** | 🔋🔋🔋 High | 🔋 Low |

### **How Easy Is It?**

**Google Maps:**
```
1. Open app
2. Wait for GPS fix (10-30 seconds)
3. See your location
4. Needs internet for maps
5. Drains battery quickly
```

**ProxiTof GPS:**
```
1. Power on device
2. Wait for GPS fix (30-60 seconds after first time)
3. See your location on OLED
4. No internet needed!
5. Logs everything automatically
6. Low battery usage
```

### **What You Get:**

**Real-Time Display:**
- Current position (lat/lon)
- Speed (km/h and m/s)
- Direction (degrees)
- Altitude (meters)
- Satellite count
- Fix quality (HDOP)

**Automatic Logging:**
- Every position saved to SD card
- With timestamp
- With sensor data (ToF, IMU)
- For later analysis
- No manual action needed!

**Offline Operation:**
- No internet required
- No phone needed
- No data charges
- Works anywhere outdoors

---

## Common Questions

### **Q: Do I need to press any button to get location?**
**A:** No! GPS updates automatically every second. Just like Google Maps, your position updates as you move.

### **Q: Will it work while I'm riding?**
**A:** Yes! GPS tracks you continuously while cycling. Speed, direction, and position update in real-time.

### **Q: Does it need internet like Google Maps?**
**A:** No! GPS receives signals directly from satellites in space. No internet, no phone, no data charges.

### **Q: Will it drain my battery?**
**A:** GPS uses about 25-67 mA (similar to a small LED). Much less than a phone running Google Maps.

### **Q: Can I see my route after riding?**
**A:** Yes! All positions are logged to CSV file. You can plot your route on a map later using the coordinates.

### **Q: What if GPS loses fix while riding?**
**A:** GPS will automatically try to reacquire. If stuck, press ACTION button in GPS mode to force refresh.

### **Q: Does it work in all countries?**
**A:** Yes! GPS is global. Plus, with all GNSS enabled (GPS+GLONASS+Galileo+BeiDou), it works worldwide.

### **Q: How accurate is it?**
**A:** 2-5 meters typically (similar to phone GPS). Better in open areas, slightly worse near buildings.

### **Q: Can I use it for navigation?**
**A:** Not directly (no maps), but you can see your heading and speed. Good for tracking, not turn-by-turn navigation.

---

## Summary

### **GPS in ProxiTof is:**

✅ **Automatic** - Updates every second, no button press needed  
✅ **Real-time** - Like Google Maps, position updates as you move  
✅ **Optimized** - Bike mode for accurate cycling tracking  
✅ **Offline** - No internet needed, works anywhere outdoors  
✅ **Logged** - Every position saved to SD card automatically  
✅ **Easy** - Just power on and go outdoors  
✅ **Reliable** - Uses 4 satellite systems (GPS+GLONASS+Galileo+BeiDou)  
✅ **Refreshable** - ACTION button to force reconnect if stuck  

### **Just like Google Maps, but:**

- ✅ No internet needed
- ✅ Lower battery usage
- ✅ Automatic logging
- ✅ More technical details (satellites, HDOP, etc.)
- ❌ No visual map (just coordinates)
- ❌ No turn-by-turn navigation

### **Perfect for:**

- 🚴 Tracking cycling routes
- 📍 Geo-tagging near-miss events
- 📊 Analyzing dangerous locations
- 🗺️ Creating safety heatmaps
- 📈 Speed and distance tracking

---

**Your GPS works just like Google Maps - automatic, real-time, and easy!** 🛰️

**Document Version**: 1.0  
**Date**: February 3, 2026  
**Status**: Complete explanation

