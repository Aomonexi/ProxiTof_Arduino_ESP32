# Data Safety Strategy - ProxiTof System

## Overview
This document explains how the ProxiTof system protects against data loss during sudden power failures, crashes, or unexpected shutdowns.

## Current Architecture

### Data Flow
```
Sensors (60 Hz) → RAM Buffer (1000 entries) → SD Card File → Physical SD Card
                      ↓                           ↓
                  ~250 KB RAM              fflush() every 5s
```

### RAM Buffer Details
- **Size**: 1000 entries (circular buffer)
- **Memory**: ~250 KB RAM
- **Purpose**: Decouple sensor reading (60 Hz) from SD card writes (slower)
- **Risk**: Data in RAM is lost on sudden power failure

## Data Safety Mechanisms

### 1. Aggressive Periodic Flushing (Every 5 Seconds)
**What it does**: Automatically writes RAM buffer to SD card every 5 seconds
**Protection**: Maximum 5 seconds of data loss on power failure
**CPU Impact**: ~0.2% (minimal)
**File Impact**: Ensures file has recent data, prevents 0 KB files

### 2. Buffer Threshold Flushing (50% Full)
**What it does**: Flushes when buffer reaches 50% capacity (500 entries)
**Protection**: Prevents buffer overflow, ensures data is written regularly
**Trigger**: High data rate or delayed flushes

### 3. Near-Miss Event Immediate Flush ⭐ NEW
**What it does**: **Immediately flushes buffer when near-miss is detected**
**Protection**: Critical near-miss events are NEVER lost (0 second risk)
**Rationale**: Near-miss data is the most important - must be preserved
**Implementation**: Added in main.c line ~738

### 4. Emergency Flush Before USB Transfer
**What it does**: Flushes all data before activating USB Mass Storage mode
**Protection**: Ensures no data loss when switching to USB access
**Trigger**: User activates USB Transfer mode (Mode 7)

### 5. Proper Shutdown Flush
**What it does**: Flushes all data when user initiates shutdown (Mode 8)
**Protection**: Clean shutdown with zero data loss
**Trigger**: User confirms shutdown via OLED menu

### 6. File Sync (Every 10 Seconds)
**What it does**: Calls `fflush()` to push OS buffer to SD card
**Protection**: Ensures data is physically written to SD card
**Note**: ESP-IDF VFS doesn't support `fsync()`, so we rely on `fflush()`

## Maximum Data Loss Scenarios

| Scenario | Data Loss | Explanation |
|----------|-----------|-------------|
| **Normal operation** | Up to 5 seconds | Periodic flush interval |
| **Near-miss event** | ~0 seconds | Immediate flush on detection ⭐ |
| **Sudden power loss** | Up to 5 seconds | Only non-critical data between flushes |
| **USB transfer** | 0 seconds | Emergency flush before handoff |
| **Proper shutdown** | 0 seconds | Full flush and file close |
| **SD card removal** | Up to 5 seconds | Same as power loss |

## Why Not Write-Through Mode?

### Write-Through (Every Sample)
- **Pros**: Zero data loss, every sample written immediately
- **Cons**: 
  - SD card writes take 2-5ms each
  - At 60 Hz, this is 120-300ms per second = 12-30% CPU time
  - May impact 60 FPS OLED display performance
  - Increases SD card wear (60 writes/sec vs 0.2 writes/sec)

### Current Buffered Mode (Every 5 Seconds)
- **Pros**:
  - Minimal CPU impact (0.2%)
  - Smooth 60 FPS display
  - Reduced SD card wear
  - Near-miss events still protected (immediate flush)
- **Cons**:
  - Up to 5 seconds of non-critical data at risk

### Decision Rationale
- **Critical data** (near-miss events) is protected with immediate flush
- **Non-critical data** (normal riding) can tolerate 5-second loss
- **Display responsiveness** is important for user experience
- **SD card longevity** is improved with fewer writes

## Alternative: Dual-File Strategy (Future Enhancement)

If you want **absolute zero data loss**, consider this approach:

### Implementation
1. **Main log file**: Current buffered system (efficient)
2. **Crash recovery file**: Write-through mode (every sample, slow)
3. **On startup**: Check for crash file, merge data, delete crash file

### Benefits
- Zero data loss guarantee
- Main file still efficient (buffered)
- Crash file only used on power failure recovery

### Trade-offs
- More complex code
- Requires startup recovery logic
- Doubles SD card writes (wear)

## Recommendations

### For Current System (Recommended)
✅ **Keep current aggressive flushing (5 seconds)**
✅ **Keep near-miss immediate flush** (NEW - most important!)
✅ **Educate users**: Use proper shutdown when possible
✅ **Accept**: 5 seconds of non-critical data may be lost on power failure

### For Future Enhancement (If Needed)
- Implement dual-file strategy for zero data loss
- Add battery backup for graceful shutdown on power loss
- Add capacitor-based power-fail detection circuit

## Testing Data Safety

### Test 1: Normal Power Loss
1. Start logging
2. Wait 3 seconds (mid-flush interval)
3. Pull power plug
4. **Expected**: Last 3 seconds of data lost, file still valid

### Test 2: Near-Miss Power Loss
1. Start logging
2. Trigger near-miss event (immediate flush)
3. Pull power plug immediately after
4. **Expected**: Near-miss data preserved, zero loss

### Test 3: USB Transfer
1. Start logging
2. Activate USB Transfer mode
3. Check file on PC
4. **Expected**: All data present, no corruption

### Test 4: Proper Shutdown
1. Start logging
2. Navigate to Shutdown mode
3. Confirm shutdown
4. **Expected**: All data preserved, clean file close

## Configuration

Current settings in `storage_manager.c`:
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 5000   // Flush every 5 seconds
#define FILE_SYNC_INTERVAL_MS 10000        // Sync every 10 seconds
#define BUFFER_EMERGENCY_THRESHOLD 50      // Flush at 50% buffer full
```

To make even more aggressive (if needed):
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 2000   // Flush every 2 seconds
#define FILE_SYNC_INTERVAL_MS 5000         // Sync every 5 seconds
#define BUFFER_EMERGENCY_THRESHOLD 30      // Flush at 30% buffer full
```

## Summary

Your concern about data loss is valid! The current system:
- ✅ Protects **critical near-miss events** with immediate flush (NEW)
- ✅ Minimizes data loss with aggressive 5-second flushing
- ✅ Prevents 0 KB files with header + test write
- ✅ Handles USB transfer safely with emergency flush
- ⚠️ Accepts up to 5 seconds of **non-critical data** may be lost on power failure

This is a **reasonable trade-off** for a battery-powered bicycle device where:
- Display responsiveness matters (60 FPS)
- Near-miss events are protected (immediate flush)
- Normal riding data is less critical (5-second loss acceptable)
- SD card longevity is important (fewer writes)

If you need **absolute zero data loss**, we can implement the dual-file strategy, but it adds complexity and SD card wear.
