/**
 * I2C Scanner - Quick diagnostic tool
 * Upload this to test I2C connections
 */

#include <Arduino.h>
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== I2C Scanner ===");
    Serial.println("Scanning I2C bus...");
    
    // Initialize I2C on GPIO 8 (SDA) and GPIO 9 (SCL)
    Wire.begin(8, 9);
    Wire.setClock(100000);  // 100kHz for testing
    
    // Enable internal pull-ups (weak, but worth trying)
    pinMode(8, INPUT_PULLUP);  // SDA
    pinMode(9, INPUT_PULLUP);  // SCL
    delay(10);
    
    // Enable ToF sensor LPn pin
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    delay(100);
    
    Serial.println("LPn pin (GPIO 4) set HIGH");
    Serial.println("\nScanning addresses 0x01-0x7F...\n");
    
    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("Device found at 0x");
            if (addr < 16) Serial.print("0");
            Serial.print(addr, HEX);
            Serial.println(" !");
            found++;
        }
    }
    
    Serial.println("\n=== Scan Complete ===");
    if (found == 0) {
        Serial.println("❌ No I2C devices found!");
        Serial.println("\nCheck:");
        Serial.println("1. SDA connected to GPIO 8");
        Serial.println("2. SCL connected to GPIO 9");
        Serial.println("3. LPn connected to GPIO 4");
        Serial.println("4. 3.3V and GND connected");
        Serial.println("5. Wires firmly seated");
    } else {
        Serial.print("✓ Found ");
        Serial.print(found);
        Serial.println(" device(s)");
        Serial.println("\nExpected: 0x29 (VL53L8CX ToF)");
    }
}

void loop() {
    delay(5000);
    Serial.println("\nPress RESET to scan again");
}
