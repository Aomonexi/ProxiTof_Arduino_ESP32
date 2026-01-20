# OLED Display Wiring - Quick Reference

## Your Display: SSD1306 128x64 (4-wire SPI, Dual-Color)

### Pin Connections

```
Display Pin  →  ESP32-S3 Pin  →  Notes
───────────     ────────────     ─────
GND          →  GND              Ground
VCC          →  3.3V             ⚠️ 3.3V ONLY (not 5V!)
SCK          →  GPIO 7           SPI Clock
SDA          →  GPIO 1           SPI Data (MOSI)
RES          →  GPIO 21          Reset
DC           →  GPIO 35          Data/Command
```

**Important:** Your display has **NO CS pin** - this is normal for 4-wire SPI!

### Dual-Color Zones

Your display has **hardware-based** color zones:

```
┌─────────────────────────────┐
│      YELLOW ZONE            │ ← Top 16 pixels
│  (Alerts & Status)          │   (rows 0-15)
├─────────────────────────────┤
│                             │
│      BLUE ZONE              │ ← Bottom 48 pixels
│  (Main Data Display)        │   (rows 16-63)
│                             │
└─────────────────────────────┘
```

The firmware automatically uses:
- **YELLOW** for warnings, alerts, and status
- **BLUE** for sensor data and measurements

### Wiring Checklist

Before powering on:

- [ ] GND → GND
- [ ] VCC → 3.3V (NOT 5V!)
- [ ] SCK → GPIO 7
- [ ] SDA → GPIO 1  
- [ ] RES → GPIO 21
- [ ] DC → GPIO 35
- [ ] No loose connections
- [ ] No short circuits

### Build and Flash

```bash
idf.py build flash monitor
```

### What You'll See

The system will show:
```
╔════════════════════════════════════════════╗
║   OLED Display (SSD1306) - PLACEHOLDER    ║
╚════════════════════════════════════════════╝

Display: 128x64 OLED (4-wire SPI mode)
Dual-color: Yellow (top 16px) + Blue (bottom 48px)

Wiring Configuration:
  Display Pin  →  ESP32-S3 Pin
  ───────────     ────────────
  GND          →  GND
  VCC          →  3.3V
  SCK          →  GPIO 7
  SDA (MOSI)   →  GPIO 1
  RES (Reset)  →  GPIO 21
  DC (D/C)     →  GPIO 35

⚠️  OLED driver is in placeholder mode
💡 Wire the display and test with hardware
```

### Display Modes (Future)

Once the driver is fully implemented:

1. **Dashboard** - ToF grid + IMU + Status
2. **ToF Focus** - Large distance readings
3. **IMU Focus** - Large orientation display
4. **Status** - System statistics

### Troubleshooting

**Display blank?**
- Check VCC is 3.3V (not 5V!)
- Verify all 6 pins are connected
- Check for loose wires

**Wrong GPIO?**
- DC pin MUST be GPIO 35
- Don't use GPIO 19 (not available on your board)

### Configuration

Edit `/sdcard/config.txt`:

```ini
OLED_ENABLED=true
OLED_REFRESH_HZ=60
OLED_BRIGHTNESS=128
OLED_MODE=0
OLED_FLIP=false
```

---

**For detailed information, see:**
- `docs/oled-4wire-spi-wiring-guide.md` - Complete wiring guide
- `docs/complete-wiring-diagram.md` - Full system wiring

**Build Status:** ✅ Compiles successfully  
**Hardware Status:** ⏳ Awaiting testing with physical display
