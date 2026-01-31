# USB Safe Disconnect - Quick Guide

## ✅ SAFE Way to Exit USB Transfer Mode

1. **On PC**: Safely eject the drive
   - Windows: Right-click drive → "Eject"
   - macOS: Drag drive to Trash or click Eject button
   - Linux: Right-click → "Safely Remove"

2. **Wait**: 1-2 seconds for PC to complete eject

3. **Disconnect**: Unplug USB cable from LEFT port (USB-OTG)

4. **Press ACTION**: Button on device to exit USB Transfer mode

5. **Done**: Device returns to normal logging mode

## ⚠️ What Happens if USB Still Connected?

If you press ACTION button while USB cable is still connected:

### OLED Display Shows:
```
USB Transfer
-----------------
!!! WARNING !!!

USB still
connected!

Disconnect first
```

### Then:
```
USB Transfer
-----------------
Deactivating...

(USB connected)
```

### Serial Console Shows:
```
W (12345) USB_MSC_SD: ⚠️  USB host still connected!
W (12345) USB_MSC_SD: ⚠️  Please disconnect USB cable from LEFT port first
W (12345) USB_MSC_SD: ⚠️  Continuing anyway - data may be corrupted if transfer in progress
```

### What Happens:
- Warning displayed for 3 seconds
- System waits 2 seconds before switching
- Mode deactivates anyway (with warnings)
- **Risk**: Data corruption if file transfer was in progress

## 🚨 NEVER Do This:
- Don't press ACTION while copying files to/from device
- Don't disconnect USB cable without ejecting first
- Don't force exit during active file transfer

## 💡 Tips:
- Always eject on PC first
- Wait for PC to confirm eject
- Then disconnect cable
- Then press ACTION button
- Check OLED for confirmation

## 🔍 How to Tell if Safe:
- **Safe**: No warning appears, smooth transition
- **Unsafe**: Warning screen shows for 3 seconds
- **Very Unsafe**: File transfer progress bar visible on PC

## Quick Troubleshooting:

**Q: Warning appears even after ejecting?**
A: Wait a few more seconds, PC might still be finalizing

**Q: Can I ignore the warning?**
A: Only if you're sure no files are being transferred

**Q: What if I accidentally exited during transfer?**
A: Check files on SD card, may need to re-copy corrupted files

**Q: How do I know if files are corrupted?**
A: Files will be 0 KB or won't open properly

## Build and Test:
```bash
idf.py build flash monitor
```

Test both scenarios:
1. Safe disconnect (eject → disconnect → ACTION)
2. Unsafe disconnect (ACTION while connected) - see warning
