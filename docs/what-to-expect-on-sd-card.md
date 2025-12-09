# What to Expect on SD Card - Quick Start Guide

## TL;DR - Quick Answer

**Q: Do we need a button to start recording?**  
**A: NO! Recording starts AUTOMATICALLY when you power on.**

**Q: What will we see on the SD card?**  
**A: Date-based folders with CSV files containing all sensor data.**

---

## Step-by-Step: What Happens When You Turn It On

### 1. Power On Sequence

```
[0ms]    ESP32-S3 boots
[100ms]  Sensors initialize (ToF, IMU)
[2000ms] SD card initializes
[2100ms] Logging starts AUTOMATICALLY ✅
         ↓
         Data is being logged to SD card!
```

### 2. Serial Monitor Output

You'll see this in the serial monitor:

```
I (1234) MAIN: ╔════════════════════════════════════════════╗
I (1234) MAIN: ║   ProxiTof Near-Miss Detection System     ║
I (1234) MAIN: ║   ESP32-S3 + VL53L8CX + BNO08x            ║
I (1234) MAIN: ╚════════════════════════════════════════════╝

I (1500) SENSOR_MGR: ✓ VL53L8CX initialized successfully (4x4 mode, 60Hz)
I (2000) SENSOR_MGR: ✓ BNO08x ready!
I (2100) MAIN: ✓ Sensor manager initialized successfully

I (2200) STORAGE_MGR: Initializing SD card via SPI...
I (2300) STORAGE_MGR: Filesystem mounted
I (2400) STORAGE_MGR: SD card mounted at /sdcard
I (2500) STORAGE_MGR: Created directory: /sdcard/2024-12-09
I (2600) STORAGE_MGR: Created log file: /sdcard/2024-12-09/log_20241209_143022.csv
I (2700) MAIN: ✓ Data logging started

[Frame: 1 | ToF: 1 @ 60Hz | IMU: 1 @ 100Hz]
LED Response: Real-time | Display: 2Hz
SD Card: Logging | Entries: 1 | Buffer: 0.8% | Dropped: 0

[Frame: 2 | ToF: 2 @ 60Hz | IMU: 3 @ 100Hz]
LED Response: Real-time | Display: 2Hz
SD Card: Logging | Entries: 2 | Buffer: 1.7% | Dropped: 0

... (continues logging automatically)
```

### 3. What You'll See on SD Card

**Immediately after power on:**

```
📁 SD Card Root
└── 📁 2024-12-09/
    └── 📄 log_20241209_143022.csv  (being written in real-time)
```

**After running for a while:**

```
📁 SD Card Root
├── 📁 2024-12-09/
│   ├── 📄 log_20241209_143022.csv  (100MB - rotated)
│   ├── 📄 log_20241209_153045.csv  (100MB - rotated)
│   └── 📄 log_20241209_163108.csv  (currently writing)
└── 📁 2024-12-10/
    └── 📄 log_20241210_084512.csv  (next day's data)
```

---

## CSV File Contents

### File Structure

**Header (first line):**
```csv
timestamp_ms,tof_00,tof_01,tof_02,tof_03,tof_10,tof_11,tof_12,tof_13,tof_20,tof_21,tof_22,tof_23,tof_30,tof_31,tof_32,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual
```

**Data rows (one per sensor reading):**
```csv
1234567,450,520,480,510,460,530,490,520,470,540,500,530,480,550,510,540,12.34,-5.67,89.12,0.12,-0.05,9.81,0.01,-0.02,0.03,0
1234583,448,518,478,508,458,528,488,518,468,538,498,528,478,548,508,538,12.35,-5.66,89.13,0.13,-0.04,9.82,0.02,-0.01,0.04,0
1234599,446,516,476,506,456,526,486,516,466,536,496,526,476,546,506,536,12.36,-5.65,89.14,0.14,-0.03,9.83,0.03,0.00,0.05,0
```

### Column Explanation

| Column | Description | Example | Unit |
|--------|-------------|---------|------|
| `timestamp_ms` | Time since boot | 1234567 | milliseconds |
| `tof_00` to `tof_33` | 4x4 distance grid | 450 | millimeters |
| `roll` | Roll angle | 12.34 | degrees |
| `pitch` | Pitch angle | -5.67 | degrees |
| `yaw` | Yaw angle | 89.12 | degrees |
| `accel_x/y/z` | Linear acceleration | 0.12 | m/s² |
| `gyro_x/y/z` | Angular velocity | 0.01 | deg/s |
| `flag_manual` | Button pressed? | 0 or 1 | boolean |

### Example Data (Human Readable)

**Normal operation (no button press):**
```csv
1234567,450,520,...,12.34,-5.67,89.12,0.12,-0.05,9.81,0.01,-0.02,0.03,0
         ↑                ↑                                              ↑
      Distance         Orientation                                  No flag
```

**Button pressed (flagging near-miss event):**
```csv
1234567,450,520,...,12.34,-5.67,89.12,0.12,-0.05,9.81,0.01,-0.02,0.03,1
         ↑                ↑                                              ↑
      Distance         Orientation                                  FLAGGED!
```

---

## Button Behavior

### What the Button Does

**The button is for FLAGGING events, not starting/stopping recording!**

- **Recording:** Starts automatically on power-up
- **Button:** Marks interesting events (near-miss) in the data

### When You Press the Button

**Serial Monitor:**
```
╔════════════════════════════════════════════════╗
║       ⚠️  BUTTON HELD - FLAGGING EVENT        ║
║   Duration: 1500 ms (1.5 seconds)             ║
║   Keep holding to mark entire near-miss event  ║
╚════════════════════════════════════════════════╝
```

**In CSV file:**
```csv
timestamp_ms,...,flag_manual
1234500,...,0          ← Before button press
1234516,...,0
1234532,...,1          ← Button pressed! (flag=1)
1234548,...,1          ← Still pressed
1234564,...,1          ← Still pressed
1234580,...,0          ← Button released
1234596,...,0
```

**Use case:** When you see a near-miss event happening, press and hold the button to mark that time period in your data for later analysis!

---

## File Size and Duration

### How Much Data?

**At 60Hz logging rate:**
- 1 second = 60 rows
- 1 minute = 3,600 rows
- 1 hour = 216,000 rows
- File size: ~1MB per minute

**File Rotation:**
- Files automatically rotate at 100MB
- ~100 minutes per file
- New file created seamlessly (no data loss)

### Example Timeline

```
00:00 - Power on
00:01 - log_20241209_143022.csv created (0 KB)
01:40 - File reaches 100MB, rotates
01:40 - log_20241209_153022.csv created (0 KB)
03:20 - File reaches 100MB, rotates
03:20 - log_20241209_163022.csv created (0 KB)
... continues until power off or SD card full
```

---

## Stopping Recording

### Option 1: Power Off (Current Implementation)

**Just turn off the ESP32-S3:**
- Buffer is flushed automatically
- File is closed properly
- Data is safe on SD card

### Option 2: Add Stop Button (Future Enhancement)

You could add a "stop logging" button if needed:
```c
if (stop_button_pressed) {
    storage_manager_stop_logging();
}
```

---

## Troubleshooting

### SD Card Shows Nothing

**Check serial monitor for:**
```
E (2200) STORAGE_MGR: Failed to mount filesystem
```

**Solutions:**
1. Format SD card as FAT32
2. Check wiring (MOSI, MISO, SCK, CS)
3. Verify 3.3V power
4. Try different SD card

### File Not Growing

**Check serial monitor for:**
```
SD Card: Logging | Entries: 0 | Buffer: 0.0% | Dropped: 0
```

**If entries = 0:**
- Sensors not reading data
- Check sensor initialization

**If buffer stuck at 100%:**
- SD write is failing
- Check SD card speed (use Class 10)

### File Stops at Certain Size

**If file stops before 100MB:**
- SD card might be full
- Check available space
- Delete old files

---

## Quick Test Procedure

### 1. Prepare SD Card
```
1. Format as FAT32
2. Insert into ESP32-S3
3. Verify wiring
```

### 2. Power On
```
1. Connect USB
2. Open serial monitor (115200 baud)
3. Watch for "✓ Data logging started"
```

### 3. Verify Logging
```
1. Wait 10 seconds
2. Check serial: "Entries: 600" (60Hz × 10s)
3. Power off
4. Remove SD card
5. Check on computer
```

### 4. Expected Result
```
📁 SD Card
└── 📁 2024-12-09/
    └── 📄 log_20241209_HHMMSS.csv (should exist!)
```

### 5. Open CSV File
```
- Should have header row
- Should have ~600 data rows (10 seconds × 60Hz)
- All columns should have values
```

---

## Data Analysis Tips

### Opening in Excel/Python

**Excel:**
```
1. Open CSV file
2. Data → Text to Columns
3. Delimiter: Comma
4. Done!
```

**Python (Pandas):**
```python
import pandas as pd

# Load data
df = pd.read_csv('log_20241209_143022.csv')

# View first few rows
print(df.head())

# Filter flagged events
flagged = df[df['flag_manual'] == 1]
print(f"Found {len(flagged)} flagged events")

# Plot distance over time
import matplotlib.pyplot as plt
plt.plot(df['timestamp_ms'], df['tof_00'])
plt.xlabel('Time (ms)')
plt.ylabel('Distance (mm)')
plt.show()
```

---

## Summary

### ✅ What Happens Automatically

- SD card initializes
- Logging starts immediately
- Data written continuously
- Files rotate at 100MB
- Buffer prevents data loss

### 🔘 What the Button Does

- Marks events in the data (flag_manual = 1)
- Does NOT start/stop recording
- Used for labeling near-miss events

### 📊 What You Get

- Date-organized folders
- Timestamped CSV files
- Full sensor data (ToF + IMU)
- Ready for ML training

### 🎯 Next Steps

1. **Build and flash** the firmware
2. **Insert SD card** (FAT32 formatted)
3. **Power on** - logging starts automatically!
4. **Press button** when you see near-miss events
5. **Collect data** for ML training
6. **Analyze** CSV files on your computer

---

**No button press needed to start - just power on and go!** 🚀
