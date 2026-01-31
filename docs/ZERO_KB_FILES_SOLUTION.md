# Zero KB Files - Prevention Solution

**Date**: January 29, 2026  
**Problem**: Some log files are 0KB (empty)  
**Cause**: Data not flushed before power loss or system crash  
**Solution**: Aggressive emergency flushing + file sync

---

## Current Emergency Flush System

The storage_manager already has emergency flush mechanisms:

### 1. Periodic Flush (Every 10 seconds)
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 10000  // 10 seconds
```

### 2. Buffer Threshold Flush (85% full)
```c
#define BUFFER_EMERGENCY_THRESHOLD 85  // 85% buffer full
```

### 3. File Sync (Every 30 seconds)
```c
#define FILE_SYNC_INTERVAL_MS 30000  // 30 seconds
```

---

## Why 0KB Files Still Occur

**Possible causes:**
1. **Power loss** before first flush (within 10 seconds of file creation)
2. **System crash** before data written
3. **SD card removed** while writing
4. **File system corruption**
5. **USB MSC mode** activated before flush

---

## Enhanced Solution

### Option 1: More Aggressive Flushing (Recommended)

**Change flush intervals:**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 5000   // 5 seconds (was 10)
#define FILE_SYNC_INTERVAL_MS 10000        // 10 seconds (was 30)
#define BUFFER_EMERGENCY_THRESHOLD 50      // 50% (was 85%)
```

**Benefits:**
- Data written every 5 seconds maximum
- Less data loss on power failure
- More frequent disk sync

**Drawbacks:**
- Slightly higher CPU usage
- More SD card wear (minimal)

### Option 2: Immediate Header Flush (Already Implemented)

The code already does this:
```c
// Write CSV header
fprintf(log_file, "timestamp_ms,...\n");

// Force immediate write of header to disk
fflush(log_file);
```

This ensures the file is never truly 0KB - it always has the header.

### Option 3: Write Test Entry (Already Implemented)

```c
// Test write to verify file is working
fprintf(log_file, "# Test write at startup - timestamp: %ld\n", (long)time(NULL));
fflush(log_file);
```

This adds a comment line to verify the file is writable.

---

## Recommended Actions

### 1. Verify Current Behavior

Check if 0KB files are:
- **Truly empty** (0 bytes)
- **Have header only** (should be ~200 bytes with header)
- **Created but never written** (file exists but no data)

### 2. Check Serial Console

Look for these messages:
```
✓ CSV header written and synced to disk (XXX bytes)
✓ Test write completed (XXX bytes total)
```

If you don't see these, the file creation is failing.

### 3. Enable More Aggressive Flushing

**File: `components/storage_manager/storage_manager.c`**

**Current (Line ~60-62):**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 10000  // Force flush every 10 seconds
#define FILE_SYNC_INTERVAL_MS 30000        // Sync file to disk every 30 seconds
#define BUFFER_EMERGENCY_THRESHOLD 85      // Emergency flush at 85% buffer full
```

**Change to:**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 5000   // Force flush every 5 seconds (more aggressive)
#define FILE_SYNC_INTERVAL_MS 10000        // Sync file to disk every 10 seconds (more aggressive)
#define BUFFER_EMERGENCY_THRESHOLD 50      // Emergency flush at 50% buffer full (more aggressive)
```

### 4. Add Flush on Mode Change

When entering USB Transfer mode, ensure data is flushed:

**File: `components/oled_display/oled_display.c`** (Line ~1256)

**Before activating USB MSC:**
```c
// Flush all data before USB MSC activation
ESP_LOGI(TAG, "Flushing data before USB MSC activation...");
storage_manager_flush();
vTaskDelay(pdMS_TO_TICKS(500));  // Wait for flush to complete
```

### 5. Add Flush on Shutdown

Already implemented in `storage_manager_safe_shutdown()`:
```c
esp_err_t storage_manager_safe_shutdown(void) {
    // Emergency flush all buffered data
    esp_err_t ret = emergency_flush_buffer();
    
    // Close current log file safely
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
    }
    
    return ESP_OK;
}
```

---

## Testing Procedure

### Test 1: Normal Operation
1. Start device
2. Let it log for 30 seconds
3. Power off (pull power)
4. Check SD card - file should have data

### Test 2: Quick Power Loss
1. Start device
2. Wait only 3 seconds
3. Power off
4. Check SD card - file should have header + test line

### Test 3: USB MSC Mode
1. Start device
2. Enter USB Transfer mode
3. Check file size before USB activation
4. File should have data

### Test 4: Shutdown Mode
1. Start device
2. Use Shutdown mode
3. Check file - should have all data

---

## File Size Expectations

**Minimum file size (empty log):**
- Header: ~200 bytes
- Test line: ~50 bytes
- **Total: ~250 bytes minimum**

**With 1 second of data (60 Hz):**
- 60 entries × ~150 bytes = ~9KB
- **Total: ~9.25KB**

**With 10 seconds of data:**
- 600 entries × ~150 bytes = ~90KB
- **Total: ~90.25KB**

---

## Diagnostic Commands

Add these to check file status:

### Check File Size
```c
struct stat st;
if (stat(current_filename, &st) == 0) {
    ESP_LOGI(TAG, "Current file size: %ld bytes", st.st_size);
}
```

### Check Buffer Status
```c
storage_stats_t stats;
storage_manager_get_stats(&stats);
ESP_LOGI(TAG, "Buffer: %.1f%% full, %lu entries logged", 
         stats.buffer_fill_percent, stats.entries_logged);
```

---

## Root Cause Analysis

If you still get 0KB files after these changes:

### Possible Issues:

1. **SD Card Failure**
   - Try different SD card
   - Check SD card health on PC
   - Format as FAT32

2. **Power Supply Issue**
   - Brownout during write
   - Insufficient current
   - Check 3.3V rail stability

3. **File System Corruption**
   - Run chkdsk/fsck on SD card
   - Reformat SD card
   - Check for bad sectors

4. **Code Path Issue**
   - File created but never written
   - Check if `storage_manager_start_logging()` succeeds
   - Check if `storage_manager_write()` is called

---

## Summary

**Current Protection:**
✅ Header written immediately  
✅ Test write after header  
✅ Periodic flush every 10 seconds  
✅ Buffer threshold flush at 85%  
✅ File sync every 30 seconds  
✅ Safe shutdown with emergency flush  

**Recommended Improvements:**
🔧 Reduce flush interval to 5 seconds  
🔧 Reduce sync interval to 10 seconds  
🔧 Lower buffer threshold to 50%  
🔧 Add flush before USB MSC activation  

**Expected Result:**
- Maximum 5 seconds of data loss on power failure
- Minimum file size: 250 bytes (header + test line)
- No more 0KB files under normal operation

---

**Next Steps:**
1. Update flush intervals in storage_manager.c
2. Rebuild and flash
3. Test with quick power cycles
4. Monitor file sizes

---

**Last Updated**: January 29, 2026  
**Status**: Solution documented, awaiting implementation  
**Author**: Kiro AI Assistant
