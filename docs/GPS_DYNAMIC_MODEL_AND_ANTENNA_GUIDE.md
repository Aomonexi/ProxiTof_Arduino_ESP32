# GPS Dynamic Model Configuration & Antenna Placement Guide

**Date**: February 4, 2026  
**Purpose**: Complete guide to GPS configuration and antenna placement  
**Status**: ✅ Implemented

---

## GPS Configuration - Already Optimal! ✅

### Multi-GNSS Support (Automatic Worldwide Coverage)

Your GPS is already configured to use **ALL available satellite systems**:

```c
// From gps.c - Already enabled!
✅ GPS (USA) - 8 channels
✅ GLONASS (Russia) - 8 channels  
✅ Galileo (Europe) - 4 channels
✅ BeiDou (China) - 8 channels
✅ SBAS (Augmentation) - 1 channel
```

**Total: 29 channels tracking up to 29 satellites simultaneously!**

### How It Works

The GPS module **automatically selects the best satellites** based on:
- Signal strength
- Satellite elevation angle
- Geographic location
- Atmospheric conditions

**You don't need to do anything** - it works worldwide automatically!

### Regional Performance

| Region | Primary System | Backup Systems | Total Satellites Visible |
|--------|---------------|----------------|-------------------------|
| **Asia** | BeiDou (14+) | GPS (8+), GLONASS (8+), Galileo (6+) | 30-40 satellites |
| **Europe** | Galileo (10+) | GPS (8+), GLONASS (8+) | 25-35 satellites |
| **Americas** | GPS (10+) | GLONASS (8+), Galileo (6+) | 25-35 satellites |
| **Russia** | GLONASS (10+) | GPS (8+), Galileo (6+) | 25-35 satellites |

**Result**: Faster fix acquisition (30-60 seconds) and better accuracy (2-5 meters)

---

## GPS Dynamic Model Configuration

### What is Dynamic Model?

The GPS module uses a **Kalman filter** to predict your position. Different movement types need different filter settings:

```
Walking:    Slow speed, gentle turns, frequent stops
Cycling:    Moderate speed, quick turns, stop-and-go
Driving:    High speed, moderate turns, rare stops
Flying:     Very high speed, very slow turns, no stops
```

### Available Models

| Model | Name | Speed Range | Best For | Tracking |
|-------|------|-------------|----------|----------|
| 0 | Portable | 0-50 km/h | General purpose | Moderate |
| 2 | Stationary | 0 km/h | Fixed position | None |
| 3 | Pedestrian | 0-10 km/h | Walking | Slow, gentle |
| 4 | Automotive | 0-100 km/h | Driving | Fast, moderate turns |
| 6 | Airborne<1g | 0-500 km/h | Flying | Very fast, slow turns |
| **10** | **Bike** | **0-30 km/h** | **Cycling** ✅ | **Quick, sharp turns** |

### Bike Mode (Recommended for Cycling Safety)

**Optimized for:**
- ✅ Quick swerves (avoiding obstacles)
- ✅ Sharp turns (urban cycling)
- ✅ Moderate speeds (0-30 km/h)
- ✅ Stop-and-go traffic
- ✅ Rapid acceleration/braking
- ✅ Evasive maneuvers

**Why it matters for near-miss detection:**
```
Portable Mode:  ╱╲  → ──  (smooths out swerve, misses event!)
Bike Mode:      ╱╲  → ╱╲  (tracks swerve accurately!)
```

### Pedestrian Mode (For Walking Data Collection)

**Optimized for:**
- ✅ Slow speeds (0-10 km/h)
- ✅ Gentle turns
- ✅ Frequent stops
- ✅ Walking patterns

**Use when:**
- Collecting pedestrian near-miss data
- Testing device while walking
- Comparing cycling vs walking safety

---

## Configuration Methods

### Method 1: Config File (Recommended)

**Edit `/sdcard/config.txt`:**

```ini
# GPS Dynamic Model Configuration
# 0=Portable, 2=Stationary, 3=Pedestrian, 4=Automotive, 6=Airborne<1g, 10=Bike
GPS_DYNAMIC_MODEL=10  # Bike mode (default)
```

**Change to Pedestrian mode:**
```ini
GPS_DYNAMIC_MODEL=3  # Pedestrian mode
```

**Steps:**
1. Remove SD card from device
2. Insert into computer
3. Edit `config.txt`
4. Save file
5. Insert SD card back
6. Restart device

**Advantage**: Persistent across power cycles, no button presses needed

### Method 2: Runtime Cycling (Coming Soon)

**In GPS Mode (Mode 4):**
- **Quick press ACTION button** → GPS Reconnect (refresh)
- **Hold ACTION button (2+ seconds)** → Enter mode cycling
  - Display shows: "GPS Mode: Bike"
  - Press ACTION again → Cycles to next mode
  - Press MODE button → Saves and exits

**Advantage**: Change mode without removing SD card

---

## Ceramic Patch Antenna Guide

### Antenna Identification

Your GPS antenna is a **ceramic patch antenna** - looks like this:

```
┌─────────────────────┐
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │ ← TOP (ceramic square)
│  ▓  CERAMIC PATCH  │    White or beige color
│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │    Smooth surface
└─────────────────────┘
         │
         │ Wire/connector
         ▼
    ┌────────┐
    │ Module │
    └────────┘
```

### Which Side is Top?

**TOP (Active Side):**
- ✅ Ceramic square (white/beige)
- ✅ Smooth, flat surface
- ✅ Usually has small label or marking
- ✅ **This side MUST face the sky**

**BOTTOM (Ground Plane):**
- Metal backing or PCB
- Wire/connector comes out this side
- Faces down toward device

### Correct Orientation

```
        ☀️ SKY
         ↑
    ┌─────────┐
    │ CERAMIC │ ← TOP faces UP
    │  (TOP)  │
    └─────────┘
         │
         │ Wire
         ▼
    ┌─────────┐
    │ Device  │
    │ PCB     │
    └─────────┘
```

**WRONG Orientation:**
```
    ┌─────────┐
    │ Device  │
    │ PCB     │
    └─────────┘
         │
         │ Wire
         ▼
    ┌─────────┐
    │ CERAMIC │ ← TOP faces DOWN (WRONG!)
    │  (TOP)  │
    └─────────┘
         ↓
        🌍 GROUND
```

---

## Antenna Placement

### Option 1: Outside Case (Best Signal) ✅ RECOMMENDED

**Advantages:**
- ✅ Best signal strength
- ✅ Fastest fix acquisition (30-60 seconds)
- ✅ Best accuracy (2-5 meters)
- ✅ Works in all conditions

**Mounting:**
```
┌──────────────────────────┐
│   Bike Handlebar         │
│                          │
│  ┌────────────────┐      │
│  │ ▓▓ ANTENNA ▓▓  │ ← Ceramic faces UP
│  │  (on top)      │
│  ├────────────────┤
│  │   Device       │
│  │   Case         │
│  └────────────────┘
│                          │
└──────────────────────────┘
```

**Protection:**
- Use waterproof tape or heat shrink
- Ensure ceramic side is exposed (not covered)
- Secure with zip ties or velcro

### Option 2: Inside Plastic Case (Acceptable)

**Advantages:**
- ✅ Protected from weather
- ✅ Cleaner appearance
- ⚠️ Slightly weaker signal (1-2 dB loss)

**Requirements:**
- ✅ Case must be **plastic** (not metal!)
- ✅ Antenna at **top** of case
- ✅ Ceramic side faces **up** (toward sky)
- ✅ No metal between antenna and sky

**Mounting:**
```
┌──────────────────────────┐
│   Plastic Case (top)     │
│  ┌────────────────┐      │
│  │ ▓▓ ANTENNA ▓▓  │ ← Inside, but at top
│  ├────────────────┤
│  │   Device       │
│  │   PCB          │
│  └────────────────┘
│                          │
└──────────────────────────┘
```

### Option 3: Inside Metal Case (NOT RECOMMENDED) ❌

**Problems:**
- ❌ Metal blocks GPS signals
- ❌ No fix or very slow fix (10+ minutes)
- ❌ Poor accuracy (10-50 meters)
- ❌ Frequent signal loss

**If you must use metal case:**
- Cut a window in the top for antenna
- Use plastic window cover
- Ensure antenna ceramic faces up through window

---

## Antenna Performance by Placement

| Placement | Fix Time | Accuracy | Signal Strength | Recommended |
|-----------|----------|----------|----------------|-------------|
| **Outside, top** | 30-60s | 2-5m | Excellent | ✅ YES |
| **Inside plastic, top** | 60-120s | 3-7m | Good | ✅ OK |
| **Inside plastic, side** | 120-300s | 5-15m | Fair | ⚠️ Marginal |
| **Inside metal** | 300s+ | 10-50m | Poor | ❌ NO |
| **Upside down** | Never | N/A | None | ❌ NO |

---

## Testing Your Antenna Placement

### Quick Test

1. **Power on device**
2. **Go outdoors** (clear sky view)
3. **Navigate to GPS Mode** (Mode 4)
4. **Wait and observe:**

**Good Placement:**
```
Time    Status              Satellites
0:00    SEARCHING          0
0:30    SEARCHING          3-5
1:00    FIX                6-8
1:30    3D FIX             8-12
```

**Poor Placement:**
```
Time    Status              Satellites
0:00    SEARCHING          0
2:00    SEARCHING          0-1
5:00    SEARCHING          1-2
10:00   FIX (maybe)        3-4
```

### Detailed Test

**Check these on OLED display (GPS Mode):**

| Metric | Good | Acceptable | Poor |
|--------|------|------------|------|
| **Fix Time** | < 60s | 60-120s | > 120s |
| **Satellites** | 8-12 | 6-8 | < 6 |
| **HDOP** | < 1.5 | 1.5-3.0 | > 3.0 |
| **Signal Strength** | Strong | Moderate | Weak |

---

## Troubleshooting

### Problem: No GPS Fix After 5 Minutes

**Check:**
1. ✅ Antenna ceramic side faces UP
2. ✅ Device is OUTDOORS (not indoors)
3. ✅ Clear view of sky (not under trees/buildings)
4. ✅ Antenna not inside metal case
5. ✅ Wiring correct (TX=38, RX=47)
6. ✅ Power supply stable (3.3V)

**Try:**
- Press ACTION button in GPS mode (reconnect)
- Move to more open area
- Wait longer (cold start can take 5-10 minutes)
- Check antenna orientation

### Problem: Fix Acquired But Poor Accuracy (HDOP > 3.0)

**Causes:**
- Antenna partially blocked
- Inside plastic case (signal attenuation)
- Near buildings (multipath)
- Few satellites visible

**Solutions:**
- Move antenna outside case
- Ensure ceramic faces up
- Move to more open area
- Wait for more satellites

### Problem: Fix Lost While Moving

**Causes:**
- Antenna orientation changed
- Moved under obstruction (bridge, tunnel)
- Wrong dynamic model (smoothing out movement)

**Solutions:**
- Secure antenna firmly
- Ensure ceramic always faces up
- Use Bike mode (not Portable)
- Avoid tunnels/covered areas

---

## Summary

### GPS Configuration ✅

**Already optimal:**
- ✅ All GNSS enabled (GPS+GLONASS+Galileo+BeiDou)
- ✅ 29 channels tracking
- ✅ Automatic satellite selection
- ✅ Works worldwide

**Dynamic Model:**
- ✅ Bike mode (10) for cycling safety
- ✅ Pedestrian mode (3) for walking data
- ✅ Configurable via config.txt
- ✅ Runtime cycling (coming soon)

### Antenna Placement ✅

**Best practice:**
- ✅ Outside case, top of device
- ✅ Ceramic side faces UP (toward sky)
- ✅ Secure mounting
- ✅ Waterproof protection

**Acceptable:**
- ✅ Inside plastic case, top position
- ✅ Ceramic side faces UP

**Avoid:**
- ❌ Inside metal case
- ❌ Ceramic side faces DOWN
- ❌ Side or bottom of device

### Expected Performance ✅

**With correct placement:**
- Fix time: 30-60 seconds (hot start)
- Accuracy: 2-5 meters
- Satellites: 8-12 visible
- HDOP: < 1.5
- Update rate: 1 Hz (1 position/second)

---

**Your GPS is already configured for optimal performance!** 🛰️

Just make sure the antenna ceramic side faces UP and you're good to go!

**Document Version**: 1.0  
**Date**: February 4, 2026  
**Status**: Complete implementation
