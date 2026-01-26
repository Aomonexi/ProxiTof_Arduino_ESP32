# FLAG Indicator Design Options

**Current Implementation:** Simple text "FLAG" on status line

---

## Current Implementation

**Dashboard Status Line:**
```
Normal:  12:34:56 LOG 3D
Pressed: 12:34:56 LOG 3D FLAG
```

**Style:** Plain text, no special formatting

**Visibility:** ⭐⭐⭐☆☆ (Okay, but could be better)

---

## Option 1: Inverted FLAG (Recommended)

**Make FLAG stand out with inverted colors**

### Visual Example

```
┌────────────────────────────────────────┐
│ 12:34:56 LOG 3D ▓▓▓▓▓▓▓▓               │ ← FLAG in inverted box
│                 ░FLAG░                  │
│                 ▓▓▓▓▓▓▓▓                │
└────────────────────────────────────────┘
```

### Implementation

**Option A: Inverted Text**
```c
// Write FLAG as inverted text
fb_write_text(0, "12:34:56 LOG 3D", false);  // Normal
fb_write_text(0, " FLAG", true);             // Inverted (at position 15)
```

**Result:**
- FLAG appears as white text on black background
- Very visible
- Matches Near Miss header style

**Visibility:** ⭐⭐⭐⭐⭐ (Excellent!)

---

## Option 2: Solid Box Around FLAG

**Draw a box around the FLAG text**

### Visual Example

```
┌────────────────────────────────────────┐
│ 12:34:56 LOG 3D ┌────┐                 │
│                 │FLAG│                  │
│                 └────┘                  │
└────────────────────────────────────────┘
```

### Implementation

```c
// Draw box using frame buffer
// Top line
for (int x = 112; x < 128; x++) {
    frame_buffer[0 * 128 + x] = 0xFF;  // Top border
}
// Bottom line
for (int x = 112; x < 128; x++) {
    frame_buffer[1 * 128 + x] = 0xFF;  // Bottom border
}
// Write FLAG text inside
fb_write_text(0, "FLAG", false);
```

**Visibility:** ⭐⭐⭐⭐☆ (Very good)

---

## Option 3: Flashing FLAG

**Make FLAG blink on/off**

### Visual Example

```
Frame 1: 12:34:56 LOG 3D FLAG
Frame 2: 12:34:56 LOG 3D 
Frame 3: 12:34:56 LOG 3D FLAG
Frame 4: 12:34:56 LOG 3D 
```

### Implementation

```c
// Flash FLAG at 2 Hz (500ms on, 500ms off)
static uint32_t last_flash = 0;
static bool flash_state = false;

if ((current_time - last_flash) >= 500) {
    flash_state = !flash_state;
    last_flash = current_time;
}

if (button_active && flash_state) {
    snprintf(status_line1, sizeof(status_line1), "%s %s %s FLAG", ...);
} else {
    snprintf(status_line1, sizeof(status_line1), "%s %s %s", ...);
}
```

**Visibility:** ⭐⭐⭐⭐⭐ (Excellent, but may be distracting)

---

## Option 4: Large FLAG Banner

**Use full line for FLAG**

### Visual Example

```
┌────────────────────────────────────────┐
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ │
│ ░░░░░░░░░░ FLAG ░░░░░░░░░░░░░░░░░░░░░ │ ← Full inverted line
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ │
│ 12:34:56 LOG 3D                        │ ← Time on line 2
└────────────────────────────────────────┘
```

### Implementation

```c
if (button_active) {
    fb_write_text(0, "   FLAGGING   ", true);  // Full inverted line
    fb_write_text(1, status_line1, false);     // Time on line 2
} else {
    fb_write_text(0, status_line1, false);     // Normal
    fb_write_text(1, status_line2, false);
}
```

**Visibility:** ⭐⭐⭐⭐⭐ (Maximum visibility!)

**Trade-off:** Uses both yellow zone lines

---

## Option 5: Icon + Text

**Add a visual icon before FLAG**

### Visual Example

```
┌────────────────────────────────────────┐
│ 12:34:56 LOG 3D ► FLAG                 │ ← Arrow icon
│ 12:34:56 LOG 3D ● FLAG                 │ ← Dot icon
│ 12:34:56 LOG 3D ⚠ FLAG                 │ ← Warning icon
└────────────────────────────────────────┘
```

### Implementation

```c
if (button_active) {
    snprintf(status_line1, sizeof(status_line1), "%s %s %s >FLAG", ...);
} else {
    snprintf(status_line1, sizeof(status_line1), "%s %s %s", ...);
}
```

**Visibility:** ⭐⭐⭐⭐☆ (Good)

---

## Comparison Table

| Option | Visibility | Distraction | Code Complexity | Recommended |
|--------|-----------|-------------|-----------------|-------------|
| **Current (Plain Text)** | ⭐⭐⭐☆☆ | Low | Very Simple | ❌ |
| **Inverted FLAG** | ⭐⭐⭐⭐⭐ | Low | Simple | ✅ Best! |
| **Box Around FLAG** | ⭐⭐⭐⭐☆ | Low | Medium | ✅ Good |
| **Flashing FLAG** | ⭐⭐⭐⭐⭐ | High | Medium | ⚠️ May annoy |
| **Large Banner** | ⭐⭐⭐⭐⭐ | Medium | Simple | ✅ Very visible |
| **Icon + Text** | ⭐⭐⭐⭐☆ | Low | Very Simple | ✅ Good |

---

## My Recommendation

### Best Option: Inverted FLAG

**Why:**
- ✅ Very visible (white on black)
- ✅ Matches Near Miss header style
- ✅ Not distracting
- ✅ Simple to implement
- ✅ Professional look

**Visual:**
```
Normal:  12:34:56 LOG 3D
Pressed: 12:34:56 LOG 3D ▓FLAG▓  ← Inverted
```

### Alternative: Large Banner (Maximum Visibility)

**Why:**
- ✅ Impossible to miss
- ✅ Very clear indication
- ✅ Professional look
- ⚠️ Uses both yellow zone lines

**Visual:**
```
Pressed: ▓▓▓▓ FLAGGING ▓▓▓▓  ← Full inverted line
         12:34:56 LOG 3D
```

---

## Implementation Code

### Option 1: Inverted FLAG (Simple)

```c
// In Dashboard mode
if (button_active) {
    // Write normal text first
    snprintf(status_line1, sizeof(status_line1), "%s %s %s ", 
             time_str, log_status, gps_status);
    fb_write_text(0, status_line1, false);
    
    // Write FLAG as inverted text at the end
    fb_write_text(0, "FLAG", true);  // Inverted at position ~13
} else {
    snprintf(status_line1, sizeof(status_line1), "%s %s %s", 
             time_str, log_status, gps_status);
    fb_write_text(0, status_line1, false);
}
```

**Problem:** fb_write_text always starts at position 0!

**Solution:** Need to modify fb_write_text to accept X position, or use a different approach.

---

### Better Implementation: Two-Part Write

```c
// Dashboard mode
if (button_active) {
    // Part 1: Normal text
    char normal_part[32];
    snprintf(normal_part, sizeof(normal_part), "%s %s %s", 
             time_str, log_status, gps_status);
    fb_write_text(0, normal_part, false);
    
    // Part 2: Inverted FLAG (manually write to frame buffer)
    const char *flag_text = "FLAG";
    int start_x = strlen(normal_part) * 8 + 8;  // After normal text + 1 space
    
    for (int i = 0; i < 4; i++) {  // "FLAG" = 4 chars
        uint8_t c = flag_text[i];
        const uint8_t *font_char = font8x8_basic_tr[c];
        
        for (int x = 0; x < 8; x++) {
            int fb_index = 0 * 128 + start_x + i * 8 + x;
            if (fb_index < FRAME_BUFFER_SIZE) {
                frame_buffer[fb_index] = ~font_char[x];  // Inverted!
            }
        }
    }
} else {
    // Normal display
    snprintf(status_line1, sizeof(status_line1), "%s %s %s", 
             time_str, log_status, gps_status);
    fb_write_text(0, status_line1, false);
}
```

---

### Simplest Implementation: Large Banner

```c
// Dashboard mode
if (button_active) {
    // Use full line 0 for FLAG banner
    fb_write_text(0, "  FLAGGING   ", true);  // Inverted, centered
    
    // Move time/status to line 1
    char status_line[32];
    snprintf(status_line, sizeof(status_line), "%s %s %s", 
             time_str, log_status, gps_status);
    fb_write_text(1, status_line, false);
    
    // Line 2: Distance (no approach rate when flagging)
    char dist_line[32];
    snprintf(dist_line, sizeof(dist_line), "%dmm", min_distance);
    fb_write_text(2, dist_line, false);  // Move to page 2 (blue zone)
} else {
    // Normal display
    fb_write_text(0, status_line1, false);
    fb_write_text(1, status_line2, false);
}
```

**This is the easiest and most visible!**

---

## What Would You Like?

**Quick Options:**

1. **Keep current** (plain text "FLAG") - Simple, works
2. **Large banner** (full inverted line) - Maximum visibility, easy to implement
3. **Inverted FLAG** (white on black) - Professional, medium complexity
4. **Flashing FLAG** (blinks on/off) - Very visible, may be annoying

**My recommendation:** **Large Banner** - Easiest to implement and impossible to miss!

