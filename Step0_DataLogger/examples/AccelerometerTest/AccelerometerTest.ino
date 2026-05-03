/**
 * @file AccelerometerTest.ino
 * @brief Simple test sketch for BMA250 accelerometer
 * @version 1.0.0
 * @date 2026-05-03
 * 
 * This sketch tests the BMA250 accelerometer connection and reads data.
 * Use this to verify your hardware setup before running the full data logger.
 */

#include <Wire.h>

// BMA250 Configuration
#define BMA250_ADDR           0x18
#define BMA250_CHIP_ID        0x00
#define BMA250_X_AXIS_LSB     0x02
#define BMA250_RANGE          0x0F
#define BMA250_BANDWIDTH      0x10
#define BMA250_POWER_MODE     0x11

// Pin definitions
#define PIN_SDA               18
#define PIN_SCL               19

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=================================");
    Serial.println("BMA250 Accelerometer Test");
    Serial.println("=================================\n");
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(400000); // 400kHz
    
    Serial.println("Checking BMA250 connection...");
    
    // Read chip ID
    uint8_t chipID = readRegister(BMA250_CHIP_ID);
    
    if (chipID == 0x03) {
        Serial.println("✓ BMA250 detected!");
        Serial.printf("  Chip ID: 0x%02X\n", chipID);
    } else {
        Serial.println("✗ BMA250 not found!");
        Serial.printf("  Expected: 0x03, Got: 0x%02X\n", chipID);
        Serial.println("\nCheck connections:");
        Serial.println("  - SDA to Pin 18");
        Serial.println("  - SCL to Pin 19");
        Serial.println("  - VCC to 3.3V");
        Serial.println("  - GND to GND");
        while(1);
    }
    
    // Configure accelerometer
    Serial.println("\nConfiguring BMA250...");
    
    writeRegister(BMA250_RANGE, 0x03);      // ±2g range
    writeRegister(BMA250_BANDWIDTH, 0x0E);  // 500Hz bandwidth
    writeRegister(BMA250_POWER_MODE, 0x00); // Normal mode
    
    delay(10);
    
    Serial.println("✓ Configuration complete");
    Serial.println("\nReading acceleration data...");
    Serial.println("Format: X, Y, Z (raw values)");
    Serial.println("Expected: X≈0, Y≈0, Z≈1024 (gravity)\n");
}

void loop() {
    // Read X, Y, Z axes
    int16_t x, y, z;
    
    if (readAccelerometer(&x, &y, &z)) {
        // Print raw values
        Serial.print("Raw: ");
        Serial.print(x);
        Serial.print(", ");
        Serial.print(y);
        Serial.print(", ");
        Serial.print(z);
        
        // Convert to g
        float x_g = x / 1024.0;
        float y_g = y / 1024.0;
        float z_g = z / 1024.0;
        
        Serial.print("  |  G: ");
        Serial.print(x_g, 3);
        Serial.print(", ");
        Serial.print(y_g, 3);
        Serial.print(", ");
        Serial.println(z_g, 3);
    } else {
        Serial.println("✗ Read error!");
    }
    
    delay(100); // 10Hz update rate
}

// Helper functions

uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(BMA250_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(BMA250_ADDR, (uint8_t)1);
    
    if (Wire.available()) {
        return Wire.read();
    }
    
    return 0;
}

void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BMA250_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

bool readAccelerometer(int16_t* x, int16_t* y, int16_t* z) {
    Wire.beginTransmission(BMA250_ADDR);
    Wire.write(BMA250_X_AXIS_LSB);
    Wire.endTransmission(false);
    
    Wire.requestFrom(BMA250_ADDR, (uint8_t)6);
    
    if (Wire.available() == 6) {
        uint8_t xl = Wire.read();
        uint8_t xh = Wire.read();
        uint8_t yl = Wire.read();
        uint8_t yh = Wire.read();
        uint8_t zl = Wire.read();
        uint8_t zh = Wire.read();
        
        // Combine bytes (10-bit resolution, left-aligned)
        *x = (int16_t)((xh << 8) | xl) >> 6;
        *y = (int16_t)((yh << 8) | yl) >> 6;
        *z = (int16_t)((zh << 8) | zl) >> 6;
        
        return true;
    }
    
    return false;
}

// Made with Bob
