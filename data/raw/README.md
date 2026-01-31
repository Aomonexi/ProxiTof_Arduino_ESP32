# Raw Training Data

**Purpose**: Store CSV files from SD card for ML training

---

## How to Use

1. **Collect Data on Device**
   - Build and flash firmware
   - Set detection mode to "Algorithm" (Mode 1)
   - Ride bike and press ACTION in Dashboard to flag near-misses
   - System logs to SD card: `/sdcard/data_YYYYMMDD_HHMMSS.csv`

2. **Copy Files Here**
   - Remove SD card from device
   - Copy all CSV files to this folder
   - Example: `data_20260131_143000.csv`

3. **Train Model**
   ```bash
   python tools/quick_start_ml.py
   ```

---

## File Format

**CSV Structure**:
```csv
timestamp_ms,flag_manual,flag_auto,tof_00,tof_01,...,roll,pitch,yaw,accel_x,...
1234567,1,0,450,523,612,701,...,12.3,-5.7,180.0,0.98,...
```

**Columns**:
- `timestamp_ms`: Milliseconds since boot
- `flag_manual`: 1 = User flagged (near-miss), 0 = Safe
- `flag_auto`: Algorithm detection (not used for training)
- `tof_00` to `tof_33`: 16 ToF distances (4×4 grid)
- `roll`, `pitch`, `yaw`: IMU orientation (degrees)
- `accel_x`, `accel_y`, `accel_z`: Acceleration (m/s²)
- `gyro_x`, `gyro_y`, `gyro_z`: Angular velocity (deg/s)

---

## Data Requirements

**Minimum**:
- 100 near-miss examples (`flag_manual=1`)
- 1000 safe examples (`flag_manual=0`)

**Recommended**:
- 200+ near-miss examples
- 2000+ safe examples
- Diverse conditions (different speeds, distances, angles)

---

## Next Steps

After copying files here:
1. Run `python tools/quick_start_ml.py`
2. Model generated in `components/ml_inference/include/model_data.h`
3. Rebuild firmware: `idf.py build flash monitor`
4. ML detection active!
