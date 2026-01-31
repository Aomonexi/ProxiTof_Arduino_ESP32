# USB Status Display - Quick Reference

## Mode 7 (USB Transfer) - Live Status Indicators

### 📊 Status: WAITING
```
┌─────────────────────┐
│ !USB MODE ACTIVE!   │ ← Yellow (Inverted)
│ Waiting for PC      │ ← Yellow (Inverted)
├─────────────────────┤
│ Status: WAITING     │
│                     │
│ Connect LEFT port   │
│ to PC               │
│                     │
│ ACTION: Exit (safe) │ ← Safe to exit!
└─────────────────────┘
```
**Meaning**: No PC connected
**Safe to exit**: ✅ YES - Press ACTION anytime
**Action**: Connect USB cable to LEFT port

---

### 📊 Status: READY
```
┌─────────────────────┐
│ !USB CONNECTED!     │ ← Yellow (Inverted)
│ PC accessing SD     │ ← Yellow (Inverted)
├─────────────────────┤
│ Status: READY       │
│                     │
│ PC can read/write   │
│                     │
│ Eject first, then   │ ← Important!
│ ACTION to exit      │
└─────────────────────┘
```
**Meaning**: PC connected and active
**Safe to exit**: ⚠️ NO - Eject on PC first!
**Action**: 
1. Eject drive on PC
2. Wait for "Status: WAITING"
3. Disconnect cable
4. Press ACTION

---

### 📊 Status: SUSPENDED
```
┌─────────────────────┐
│ !USB SUSPENDED!     │ ← Yellow (Inverted)
│ PC idle/sleep       │ ← Yellow (Inverted)
├─────────────────────┤
│ Status: SUSPENDED   │
│                     │
│ PC idle or sleep    │
│ Wake PC to access   │
│                     │
│ ACTION: Exit        │
└─────────────────────┘
```
**Meaning**: PC connected but in sleep/idle mode
**Safe to exit**: ⚠️ PROBABLY - Eject first if possible
**Action**: Wake PC or exit mode

---

## Quick Decision Guide

### When Can I Press ACTION to Exit?

| OLED Shows | Safe to Exit? | What to Do |
|------------|---------------|------------|
| **Status: WAITING** | ✅ YES | Press ACTION immediately |
| **Status: READY** | ❌ NO | Eject on PC first, then ACTION |
| **Status: SUSPENDED** | ⚠️ MAYBE | Eject if possible, then ACTION |

---

## Step-by-Step: Safe Exit

### ✅ Recommended Procedure
```
1. Look at OLED display
   ↓
2. Is it "Status: READY"?
   ├─ YES → Go to step 3
   └─ NO → Go to step 7
   ↓
3. On PC: Eject the drive
   - Windows: Right-click → "Eject"
   - macOS: Drag to Trash
   - Linux: Right-click → "Safely Remove"
   ↓
4. Wait 1-2 seconds
   ↓
5. Check OLED: Should change to "Status: WAITING"
   ↓
6. Disconnect USB cable from LEFT port
   ↓
7. Press ACTION button on device
   ↓
8. Done! ✅ No warning, safe exit
```

### ⚠️ Quick Exit (If Urgent)
```
1. Press ACTION button
   ↓
2. If warning appears:
   - You know PC was connected
   - System exits anyway
   - Check files later for corruption
   ↓
3. Done (but risky if files were transferring)
```

---

## What Each Status Means

### WAITING = No PC Connected
- USB cable not plugged in, OR
- PC hasn't recognized device yet, OR
- PC has ejected the drive

**What you can do**:
- Connect USB cable to see files
- Press ACTION to exit (100% safe)

### READY = PC Can Access Files
- PC has mounted the drive
- PC can read/write files
- File transfers possible

**What you can do**:
- Copy files to/from device
- View files on PC
- Must eject before exiting!

### SUSPENDED = PC Idle/Sleep
- PC is in low-power mode
- No active file transfers
- USB bus suspended

**What you can do**:
- Wake PC to access files
- Exit mode (probably safe)
- Eject first if possible

---

## Real-Time Updates

The OLED display updates **every 100ms**, so you'll see status changes almost immediately:

- Plug in USB cable → "WAITING" → "READY" (within 1 second)
- Eject on PC → "READY" → "WAITING" (within 1 second)
- PC goes to sleep → "READY" → "SUSPENDED" (within 1 second)
- Wake PC → "SUSPENDED" → "READY" (within 1 second)

---

## Warning Screen (If You Exit While Connected)

If you press ACTION while "Status: READY", you'll see:

```
┌─────────────────────┐
│ USB Transfer        │
├─────────────────────┤
│ !!! WARNING !!!     │
│                     │
│ USB still           │
│ connected!          │
│                     │
│ Disconnect first    │
└─────────────────────┘
```

This appears for **3 seconds**, then mode exits anyway.

**Risk**: Data corruption if file transfer was in progress!

---

## Tips

### ✅ DO:
- Watch the OLED status before exiting
- Wait for "Status: WAITING" before pressing ACTION
- Eject on PC when "Status: READY"
- Trust the live status display

### ❌ DON'T:
- Press ACTION when "Status: READY" without ejecting
- Disconnect cable when "Status: READY"
- Ignore the status display
- Exit during active file transfer

---

## Troubleshooting

**Q: Status stuck on "WAITING" after connecting USB?**
A: Try different USB cable or port. Check if PC recognizes device.

**Q: Status shows "READY" but PC doesn't show drive?**
A: Restart PC or try different USB port. Check Device Manager (Windows).

**Q: Status changes too fast to read?**
A: Normal! Status updates in real-time. Watch for stable state.

**Q: Can I trust "Status: WAITING" to exit safely?**
A: Yes! If it says WAITING, no PC is connected. 100% safe to exit.

**Q: What if I'm not sure?**
A: Wait for "Status: WAITING" before exiting. Better safe than sorry!

---

## Build and Test

```bash
idf.py build flash monitor
```

Then:
1. Navigate to Mode 7 (USB Transfer)
2. Press ACTION to activate
3. Watch OLED status change as you connect/disconnect USB
4. Test safe exit (eject first, then ACTION)
5. Test unsafe exit (ACTION while connected) - see warning

---

**Remember**: The OLED tells you exactly when it's safe to exit! 🎯
