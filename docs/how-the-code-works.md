# How ProxiTof Code Works - Simple Explanation

**Date:** January 26, 2026  
**Audience:** Understanding the code architecture  
**Level:** Beginner-friendly

---

## The Big Picture

Think of your ProxiTof system like a **restaurant**:

- **main.c** = The **head chef** (coordinates everything)
- **components/** = The **kitchen stations** (each does one job well)
- **ESP-IDF** = The **kitchen equipment** (provided by Espressif)

```
┌─────────────────────────────────────────────────┐
│                   main.c                        │
│              (The Head Chef)                    │
│  "Okay, read sensors, update display, log data"│
└─────────────────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        ↓             ↓             ↓
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│sensor_manager│ │oled_display  │ │storage_manager│
│(Sensor Chef) │ │(Display Chef)│ │(Storage Chef) │
│"Read ToF/IMU"│ │"Show data"   │ │"Save to SD"   │
└──────────────┘ └──────────────┘ └──────────────┘
        ↓             ↓             ↓
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Hardware   │ │   Hardware   │ │   Hardware   │
│ ToF + IMU    │ │ OLED Screen  │ │   SD Card    │
└──────────────┘ └──────────────┘ └──────────────┘
```

---

## How It Actually Works

### Step 1: Startup (app_main in main.c)

When you power on the device, **main.c** starts and says:

```c
void app_main(void) {
    // "Hey everyone, wake up and get ready!"
    
    // 1. Initialize sensor manager
    sensor_manager_init();  // "Sensor chef, set up your station!"
    
    // 2. Initialize display
    oled_display_init();    // "Display chef, turn on the screen!"
    
    // 3. Initialize storage
    storage_manager_init(); // "Storage chef, prepare the SD card!"
    
    // 4. Initialize time sync
    smart_time_init();      // "Time keeper, sync the clock!"
    
    // Now everyone is ready, let's start the main loop!
}
```

**What happens:**
- main.c **calls functions** from each component
- Each component **initializes its hardware**
- Components return "OK" or "ERROR"
- main.c checks if everything is ready

---

### Step 2: Main Loop (runs forever)

After initialization, main.c enters an **infinite loop** that runs 60 times per second:

```c
while (1) {  // Loop forever!
    
    // 1. Read sensors
    tof_data_t tof = sensor_manager_read_tof();
    imu_data_t imu = sensor_manager_read_imu();
    
    // 2. Update display
    oled_display_update(&tof, &imu, min_distance);
    
    // 3. Save to SD card
    storage_manager_write(&tof, &imu);
    
    // 4. Check buttons
    check_mode_button();
    
    // 5. Wait 16ms (60 FPS)
    vTaskDelay(pdMS_TO_TICKS(16));
}
```

**Think of it like this:**
```
main.c: "Sensor manager, give me the latest data!"
sensor_manager: "Here's the ToF and IMU data!"

main.c: "Display manager, show this data!"
oled_display: "Done! Updated the screen!"

main.c: "Storage manager, save this!"
storage_manager: "Saved to SD card!"

main.c: "Okay, wait 16ms and do it all again!"
```

---

## Component Architecture

### What is a Component?

A **component** is like a **mini-program** that does **one job** really well.

**Example: sensor_manager component**

```
components/sensor_manager/
├── include/
│   └── sensor_manager.h      ← "Menu" (what functions you can call)
├── sensor_manager.cpp         ← "Kitchen" (how it actually works)
└── CMakeLists.txt            ← "Recipe" (how to build it)
```

**The Header File (.h) - The "Menu"**

This tells main.c what functions are available:

```c
// sensor_manager.h
#pragma once

// Functions you can call from main.c:
esp_err_t sensor_manager_init(void);           // Initialize sensors
tof_data_t sensor_manager_read_tof(void);      // Read ToF sensor
imu_data_t sensor_manager_read_imu(void);      // Read IMU sensor
bool sensor_manager_is_button_pressed(void);   // Check button
```

**The Implementation File (.cpp) - The "Kitchen"**

This is where the actual work happens:

```cpp
// sensor_manager.cpp
#include "sensor_manager.h"

esp_err_t sensor_manager_init(void) {
    // Set up I2C bus
    // Initialize ToF sensor
    // Initialize IMU sensor
    // Initialize button GPIO
    return ESP_OK;
}

tof_data_t sensor_manager_read_tof(void) {
    // Talk to ToF sensor over I2C
    // Read 16 distance values
    // Return the data
}
```

---

## How main.c Calls Components

### Example 1: Reading Sensors

**In main.c:**
```c
#include "sensor_manager.h"  // Include the "menu"

void app_main(void) {
    // Call the function from sensor_manager
    tof_data_t tof_data = sensor_manager_read_tof();
    
    // Now you have the data!
    printf("Distance: %d mm\n", tof_data.distances[0]);
}
```

**What happens behind the scenes:**
1. main.c calls `sensor_manager_read_tof()`
2. Code jumps to sensor_manager.cpp
3. sensor_manager talks to hardware over I2C
4. Gets data from ToF sensor
5. Returns data back to main.c
6. main.c continues with the data

---

### Example 2: Updating Display

**In main.c:**
```c
#include "oled_display.h"  // Include the "menu"

void app_main(void) {
    // Get sensor data
    tof_data_t tof = sensor_manager_read_tof();
    imu_data_t imu = sensor_manager_read_imu();
    
    // Tell display to update
    oled_display_update(&tof, &imu, min_distance);
    
    // Display is now updated!
}
```

**What happens:**
1. main.c calls `oled_display_update()`
2. Passes sensor data as parameters
3. oled_display.c receives the data
4. Draws graphics on screen
5. Returns to main.c

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                        main.c                           │
│                    (Orchestrator)                       │
└─────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ↓                   ↓                   ↓
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│sensor_manager │   │ oled_display  │   │storage_manager│
│               │   │               │   │               │
│ read_tof()    │   │ update()      │   │ write()       │
│ read_imu()    │   │ set_mode()    │   │ get_stats()   │
│ is_button()   │   │ handle_btn()  │   │               │
└───────────────┘   └───────────────┘   └───────────────┘
        │                   │                   │
        ↓                   ↓                   ↓
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│   Hardware    │   │   Hardware    │   │   Hardware    │
│               │   │               │   │               │
│ VL53L8CX ToF  │   │ SSD1306 OLED  │   │   SD Card     │
│ BNO08x IMU    │   │ 128x64 pixels │   │   FAT32       │
│ Button GPIO 3 │   │ SPI interface │   │   SDMMC       │
└───────────────┘   └───────────────┘   └───────────────┘
```

---

## Real Example: Button Press Flow

Let's trace what happens when you press Button 2 (GPIO 3):

### Step 1: Hardware Level
```
You press button → GPIO 3 goes LOW (0V)
```

### Step 2: sensor_manager detects it
```c
// In sensor_manager.cpp
bool sensor_manager_is_button_pressed(void) {
    bool current_state = (gpio_get_level(FLAG_BUTTON_PIN) == 0);
    // Debouncing logic...
    return button_was_pressed;
}
```

### Step 3: main.c checks button
```c
// In main.c main loop
if (sensor_manager_is_button_pressed()) {
    // Button was pressed!
    
    // Check current display mode
    oled_display_mode_t mode = oled_display_get_mode();
    
    if (mode == OLED_MODE_DASHBOARD) {
        // Flag event in log
        log_entry.flag_manual = 1;
        ESP_LOGI(TAG, "✓ EVENT FLAGGED");
    }
    else if (mode == OLED_MODE_WIFI_SETTINGS) {
        // Trigger WiFi sync
        oled_display_handle_action_button();
    }
}
```

### Step 4: Action happens
```c
// If WiFi Settings mode, oled_display.c handles it:
esp_err_t oled_display_handle_action_button(void) {
    // Show "Connecting..." on screen
    // Call smart_time_sync_now()
    // Show result
}
```

**Complete flow:**
```
Button Press (Hardware)
    ↓
sensor_manager detects (Component)
    ↓
main.c checks button (Main)
    ↓
main.c decides action based on mode (Main)
    ↓
Calls appropriate component function (Component)
    ↓
Action completed (Hardware)
```

---

## Why This Architecture?

### Benefits of Components

**1. Organization**
- Each component has one job
- Easy to find code
- Clear responsibilities

**2. Reusability**
- sensor_manager can be used in other projects
- Just copy the component folder!

**3. Testing**
- Test each component separately
- Find bugs faster

**4. Teamwork**
- Different people can work on different components
- Less conflicts

**5. Maintenance**
- Fix bugs in one place
- Changes don't break other parts

---

## Component Communication

### How Components Talk to Each Other

**Method 1: Through main.c (Most Common)**

```c
// main.c acts as the middleman
tof_data_t tof = sensor_manager_read_tof();  // Get from sensor_manager
oled_display_update(&tof, ...);              // Give to oled_display
```

**Method 2: Direct Include (Less Common)**

```c
// In oled_display.c
#include "smart_time.h"  // Include another component

void oled_display_update(...) {
    // Can call smart_time functions directly
    struct tm timeinfo;
    smart_time_get(&timeinfo);
}
```

**Method 3: Shared Data Structures**

```c
// In include/DataModels.h (shared by everyone)
typedef struct {
    uint16_t distances[16];
    uint32_t timestamp_ms;
    bool valid;
} tof_data_t;

// Now everyone uses the same structure!
```

---

## Build Process (How It All Comes Together)

### Step 1: Compile Each Component

```bash
idf.py build
```

**What happens:**
```
1. Compile sensor_manager.cpp → sensor_manager.o
2. Compile oled_display.c → oled_display.o
3. Compile storage_manager.c → storage_manager.o
4. Compile smart_time.c → smart_time.o
5. Compile main.c → main.o
```

### Step 2: Link Everything Together

```
sensor_manager.o  ┐
oled_display.o    ├─→ Linker ─→ proxitof.elf (executable)
storage_manager.o │
smart_time.o      │
main.o            ┘
```

### Step 3: Flash to ESP32

```bash
idf.py flash
```

**Result:** One complete program running on ESP32!

---

## Analogy: Restaurant Kitchen

Let me use a restaurant analogy to make it super clear:

### The Restaurant (Your ProxiTof System)

**Head Chef (main.c):**
- Coordinates everything
- Tells each station what to do
- Makes sure orders are completed

**Stations (Components):**

1. **Prep Station (sensor_manager)**
   - Job: Prepare ingredients (read sensors)
   - Tools: I2C, SPI, GPIO
   - Output: Fresh data

2. **Display Station (oled_display)**
   - Job: Plate the food (show data)
   - Tools: SPI display
   - Output: Beautiful presentation

3. **Storage Station (storage_manager)**
   - Job: Store leftovers (save to SD)
   - Tools: SDMMC interface
   - Output: Saved files

4. **Time Keeper (smart_time)**
   - Job: Keep track of time
   - Tools: RTC, WiFi, NTP
   - Output: Current time

### A Customer Order (One Loop Iteration)

```
Customer: "I want to see current sensor data!"

Head Chef (main.c): 
  "Prep station, get me fresh sensor readings!"
  
Prep Station (sensor_manager):
  "Here's ToF and IMU data!"
  
Head Chef:
  "Display station, plate this data nicely!"
  
Display Station (oled_display):
  "Done! Showing on screen!"
  
Head Chef:
  "Storage station, save this for records!"
  
Storage Station (storage_manager):
  "Saved to SD card!"
  
Head Chef:
  "Order complete! Next customer in 16ms!"
```

---

## Quick Reference

### How to Use a Component in main.c

**Step 1: Include the header**
```c
#include "component_name.h"
```

**Step 2: Initialize it**
```c
component_init();
```

**Step 3: Use its functions**
```c
data = component_read_data();
component_do_something(data);
```

### How to Create a New Component

**Step 1: Create folder**
```
components/my_component/
```

**Step 2: Create header file**
```c
// include/my_component.h
#pragma once
esp_err_t my_component_init(void);
void my_component_do_something(void);
```

**Step 3: Create implementation**
```c
// my_component.c
#include "my_component.h"

esp_err_t my_component_init(void) {
    // Initialize hardware
    return ESP_OK;
}

void my_component_do_something(void) {
    // Do the work
}
```

**Step 4: Create CMakeLists.txt**
```cmake
idf_component_register(
    SRCS "my_component.c"
    INCLUDE_DIRS "include"
)
```

**Step 5: Use in main.c**
```c
#include "my_component.h"

void app_main(void) {
    my_component_init();
    my_component_do_something();
}
```

---

## Summary

**The Simple Answer:**

> **main.c is the boss** that tells each component what to do.
> 
> **Components are workers** that do specific jobs.
> 
> **They communicate** by calling each other's functions.
> 
> **Everything runs in a loop** 60 times per second.

**The Flow:**
```
main.c → "Hey sensor_manager, read sensors!"
sensor_manager → "Here's the data!"
main.c → "Hey oled_display, show this!"
oled_display → "Done!"
main.c → "Hey storage_manager, save this!"
storage_manager → "Saved!"
main.c → "Okay, wait 16ms and repeat!"
```

**That's it!** 🎉

---

**Questions?**
- "How does main.c know what functions to call?" → It reads the .h header files
- "How do components talk to hardware?" → They use ESP-IDF drivers (I2C, SPI, GPIO)
- "Can components call each other?" → Yes, but usually through main.c
- "What if a component fails?" → It returns an error code, main.c handles it

