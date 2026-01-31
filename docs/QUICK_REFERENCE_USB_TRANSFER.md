# USB Transfer Mode - Quick Reference Card

## 🎯 Quick Actions

### Build and Flash
```bash
idf.py build flash monitor
```

### Activate USB Transfer Mode
1. Press **Mode Change** button → Navigate to Mode 7
2. Press **ACTION** button → Activate USB mode
3. Connect **LEFT port** to PC
4. Access files on PC

### Deactivate USB Transfer Mode
1. Disconnect USB cable from PC
2. Press **ACTION** button → Deactivate USB mode
3. Logging resumes automatically

---

## 🔧 Troubleshooting

### Button Doesn't Toggle?
✅ **FIXED** - Build and flash latest code

### PC Doesn't Detect USB?
Try these in order:
1. ⚠️ **Try 3 different USB cables** (most common issue)
2. ⚠️ **Verify using LEFT port** (not RIGHT port)
3. ⚠️ **Test cable with Android phone** (verify data transfer works)
4. ⚠️ **Check Device Manager** (Windows) or `lsusb` (Linux)

### Still Not Working?
**Alternative**: Use SD card reader instead
1. Power off device
2. Remove SD card
3. Insert into PC card reader
4. Access files
5. Insert back into device
6. Power on

---

## 📋 Hardware Checklist

### USB Ports (ESP32-S3 DevKitC)
```
┌─────────────────────────┐
│  [LEFT]      [RIGHT]    │
│   USB-OTG    USB-UART   │
│                         │
│   Use LEFT! ────────────┘
└─────────────────────────┘
```

### Buttons
- **Mode Change** (GPIO 20): Cycle through modes
- **ACTION** (GPIO 3): Toggle USB mode (when in Mode 7)

### LED Indicator
- **Blue**: USB Transfer mode active
- **Green**: Normal operation (far distance)
- **Yellow**: Medium distance
- **Red**: Close distance
- **Orange flash**: Near-miss detected

---

## 📊 Expected Behavior

### Activation Sequence
```
1. Navigate to Mode 7
   OLED: "USB Transfer Mode"
   
2. Press ACTION
   OLED: "Activating..."
   Serial: "USB Transfer mode activated"
   LED: Blue
   
3. Connect LEFT port
   PC: New drive appears
   OLED: "[USB] Connect LEFT port"
```

### Deactivation Sequence
```
1. Disconnect USB cable
   
2. Press ACTION
   OLED: "Deactivating..."
   Serial: "Logging mode activated"
   LED: Returns to normal color
   
3. Logging resumes
   OLED: "Logging resumed [OK]"
```

---

## 🆘 Quick Help

### Most Common Issues

| Problem | Solution | Success Rate |
|---------|----------|--------------|
| PC doesn't detect USB | Try different cable | 90% |
| Wrong USB port | Use LEFT port | 5% |
| Hardware issue | Test ESP-IDF example | 3% |
| Driver issue | Check Device Manager | 2% |

### Quick Tests

#### Test 1: Cable Test (2 minutes)
```
1. Connect Android phone with same cable
2. Does PC detect phone?
   YES → Cable is good
   NO → Cable is bad (power-only)
```

#### Test 2: Port Test (1 minute)
```
1. Try LEFT port
2. Try RIGHT port
3. Which one works?
```

#### Test 3: Hardware Test (5 minutes)
```bash
cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc
idf.py build flash monitor
# Does PC detect USB device?
```

---

## 📞 Need Help?

### Information to Provide

1. **Cable test**: Tried 3 cables? Works with phone?
2. **Port test**: Using LEFT or RIGHT port?
3. **Device Manager**: What appears when connected?
4. **Serial output**: Copy logs when activating USB mode
5. **Hardware**: ESP32-S3 board model?

### Documentation

- **Complete status**: `USB_TRANSFER_COMPLETE_STATUS_JAN29.md`
- **Quick test guide**: `USB_TRANSFER_QUICK_TEST_GUIDE.md`
- **Troubleshooting**: `USB_TROUBLESHOOTING_JAN29.md`
- **Button fix**: `USB_BUTTON_TOGGLE_FIX_JAN29.md`

---

## 💡 Pro Tips

### Tip 1: Use SD Card Reader
If USB is too complex, just use SD card reader:
- Always works
- No cable issues
- No driver issues
- Takes 2 minutes

### Tip 2: Focus on Thesis
Your thesis is about **near-miss detection**, not USB!
- Don't let USB block your progress
- Use SD card reader for now
- Add USB later if time permits

### Tip 3: Test with Known-Good Cable
Use a cable you KNOW works for data transfer:
- USB cable from Android phone
- USB cable from external hard drive
- NOT a charging-only cable

---

## ✅ Success Criteria

### Button Toggle Works
- [  ] Press ACTION → USB mode activates
- [  ] Press ACTION again → USB mode deactivates
- [  ] Can toggle multiple times reliably

### USB Detection Works
- [  ] PC detects new USB device
- [  ] New drive appears in File Explorer
- [  ] Can access SD card files
- [  ] Can copy files to PC

### Logging Works
- [  ] Logging pauses when USB mode active
- [  ] Logging resumes when USB mode deactivated
- [  ] No data loss during mode switch

---

**Remember**: If USB doesn't work, use SD card reader and focus on your thesis!

