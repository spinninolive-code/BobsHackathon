# Phase A Data Logger - Build and Test Guide

## Overview

This document provides instructions for building, flashing, and testing the Phase A ESP32-S3 Data Logger firmware.

## Prerequisites

### Hardware Requirements
- Unexpected Maker ProS3 16MB board (ESP32-S3)
- Two 10-DOF sensor modules (ICM-42688-P, MMC5983MA, LPS22HB)
- SD card module (Adafruit #4682)
- 220mAh LiPo battery
- DIP switches (3-position)
- USB-C cable for programming

### Software Requirements
- ESP-IDF v5.1 or later
- Python 3.8 or later
- Git

## Installation

### 1. Install ESP-IDF

```bash
# Clone ESP-IDF
git clone -b v5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3

# Set up environment (add to ~/.bashrc or run each session)
. ./export.sh
```

### 2. Clone Project

```bash
cd ~/projects
git clone <repository-url>
cd Step1_PhaseA_DataLogger
```

## Building

### 1. Configure Project

```bash
# Set target to ESP32-S3
idf.py set-target esp32s3

# Optional: Configure via menuconfig
idf.py menuconfig
```

Key configuration options:
- **Component config → ESP32S3-Specific → Support for external, SPI-connected RAM**: Enable
- **Component config → ESP32S3-Specific → SPI RAM config → Set RAM clock speed**: 80MHz
- **Component config → FreeRTOS → Kernel tick frequency**: 1000Hz

### 2. Build Firmware

```bash
# Full build
idf.py build

# Clean build (if needed)
idf.py fullclean
idf.py build
```

Build output will be in `build/` directory.

## Flashing

### 1. Connect Hardware

1. Connect ESP32-S3 board via USB-C
2. Verify port detection:
   ```bash
   ls /dev/ttyUSB* # Linux
   ls /dev/cu.* # macOS
   ```

### 2. Flash Firmware

```bash
# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Flash only
idf.py -p /dev/ttyUSB0 flash

# Erase flash first (recommended for first flash)
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```

### 3. Monitor Serial Output

```bash
# Monitor only
idf.py -p /dev/ttyUSB0 monitor

# Exit monitor: Ctrl+]
```

## Hardware Setup

### Pin Connections

#### Sensor Module 1 (SPI)
- CS: GPIO 10
- MOSI: GPIO 35
- MISO: GPIO 37
- SCK: GPIO 36

#### Sensor Module 2 (SPI)
- CS: GPIO 11
- MOSI: GPIO 35 (shared)
- MISO: GPIO 37 (shared)
- SCK: GPIO 36 (shared)

#### SD Card (SDIO 4-bit)
- CMD: GPIO 38
- CLK: GPIO 39
- D0: GPIO 40
- D1: GPIO 41
- D2: GPIO 42
- D3: GPIO 43

#### DIP Switches
- SW1: GPIO 4
- SW2: GPIO 5
- SW3: GPIO 6

#### LED
- WS2812: GPIO 48

#### Battery Monitor
- ADC: GPIO 1

### DIP Switch Modes

| SW3 | SW2 | SW1 | Mode | Description |
|-----|-----|-----|------|-------------|
| 0   | 0   | 0   | IDLE | System idle, no acquisition |
| 0   | 0   | 1   | ACQUIRE | Acquire data to buffer only |
| 0   | 1   | 0   | STORE | Store buffered data to SD |
| 0   | 1   | 1   | ACQUIRE_STORE | Acquire and store continuously |
| 1   | 0   | 0   | TEST | Test mode |
| 1   | 1   | 1   | SHUTDOWN | Shutdown system |

## Testing

### Test 1: System Initialization

**Objective**: Verify all components initialize correctly

**Procedure**:
1. Flash firmware
2. Monitor serial output
3. Verify initialization messages:
   ```
   I (xxx) MAIN: Initializing system...
   I (xxx) SENSOR: Initializing sensor drivers...
   I (xxx) ICM42688: ICM-42688-P initialized (WHO_AM_I: 0x47)
   I (xxx) BUFFER: Buffer manager initialized
   I (xxx) SD_STORAGE: SD card initialized: X.XX GB total
   I (xxx) STATE_MACHINE: State machine initialized
   I (xxx) POWER: Power manager initialized
   I (xxx) UI_MANAGER: UI manager initialized
   ```

**Expected Result**: All components initialize without errors

### Test 2: Sensor Data Acquisition

**Objective**: Verify 1kHz sensor sampling

**Procedure**:
1. Set DIP switches to ACQUIRE mode (001)
2. Monitor serial output for sensor data
3. Verify sample rate using timestamps

**Expected Result**: 
- Sensor data logged at 1kHz (1ms intervals)
- No data loss warnings
- Accelerometer, gyroscope, magnetometer, pressure data present

### Test 3: Buffer Management

**Objective**: Verify multi-tier buffering (PSRAM → NAND → SD)

**Procedure**:
1. Set DIP switches to ACQUIRE mode
2. Let system run for 30 seconds
3. Check buffer status in serial output
4. Set to STORE mode to flush buffers

**Expected Result**:
- PSRAM buffer fills first
- NAND cache used when PSRAM >50% full
- No buffer overflow warnings
- Data successfully written to SD card

### Test 4: SD Card Storage

**Objective**: Verify CSV file creation and data storage

**Procedure**:
1. Insert formatted SD card (FAT32)
2. Set DIP switches to ACQUIRE_STORE mode (011)
3. Let system run for 60 seconds
4. Set to IDLE mode
5. Remove SD card and check files

**Expected Result**:
- CSV file created with timestamp name (e.g., `data_20260503_143000.csv`)
- CSV header present
- Data rows with correct format
- No corrupted data

### Test 5: DIP Switch Control

**Objective**: Verify mode switching via DIP switches

**Procedure**:
1. Start in IDLE mode (000)
2. Switch to ACQUIRE mode (001)
3. Switch to STORE mode (010)
4. Switch to ACQUIRE_STORE mode (011)
5. Monitor state transitions

**Expected Result**:
- State machine transitions correctly
- LED color changes for each state
- No invalid transitions

### Test 6: LED Indicators

**Objective**: Verify LED status indication

**Procedure**:
1. Observe LED in each mode:
   - IDLE: Green solid
   - ACQUIRING: Blue slow blink
   - STORING: Cyan fast blink
   - ERROR: Red fast blink

**Expected Result**: LED colors and patterns match system state

### Test 7: Battery Monitoring

**Objective**: Verify battery voltage monitoring

**Procedure**:
1. Connect battery
2. Monitor serial output for battery status
3. Check voltage and percentage readings

**Expected Result**:
- Battery voltage reported correctly
- Percentage calculated (0-100%)
- Low battery warning at <3.5V

### Test 8: Performance Benchmarks

**Objective**: Verify system meets performance requirements

**Procedure**:
1. Run in ACQUIRE_STORE mode for 10 minutes
2. Monitor CPU usage, memory usage, sample rate
3. Check for data loss

**Expected Result**:
- Sample rate: 1000 Hz ±1%
- CPU usage: <80%
- PSRAM usage: <4MB
- Zero data loss
- SD write latency: <10ms

## Troubleshooting

### Issue: SD Card Not Detected

**Solutions**:
- Check SD card format (must be FAT32)
- Verify SDIO pin connections
- Try different SD card
- Check SD card capacity (max 32GB for FAT32)

### Issue: Sensor Not Responding

**Solutions**:
- Verify SPI pin connections
- Check sensor power supply (3.3V)
- Verify WHO_AM_I register reads correctly
- Check for SPI bus conflicts

### Issue: Data Loss Warnings

**Solutions**:
- Reduce sample rate if needed
- Increase buffer sizes in `app_config.h`
- Optimize SD card write speed
- Check for task priority issues

### Issue: Build Errors

**Solutions**:
- Verify ESP-IDF version (v5.1+)
- Run `idf.py fullclean` and rebuild
- Check component dependencies
- Update submodules: `git submodule update --init --recursive`

### Issue: Flash Errors

**Solutions**:
- Erase flash first: `idf.py erase-flash`
- Check USB cable and connection
- Try different USB port
- Verify board is in download mode

## Performance Optimization

### CPU Optimization
- Sensor task on Core 1 (dedicated)
- Data processing on Core 0
- Use DMA for SPI transfers
- Minimize ISR execution time

### Memory Optimization
- Use PSRAM for large buffers
- Minimize heap fragmentation
- Monitor stack usage
- Use static allocation where possible

### Power Optimization
- Use light sleep when idle
- Reduce LED brightness
- Optimize sensor sample rates
- Disable unused peripherals

## Data Analysis

### CSV File Format

```csv
Timestamp_ms,Module,Sensor,Accel_X_g,Accel_Y_g,Accel_Z_g,Gyro_X_dps,Gyro_Y_dps,Gyro_Z_dps,Mag_X_uT,Mag_Y_uT,Mag_Z_uT,Pressure_hPa,Temperature_C
1000,1,ACCEL,0.123,-0.456,0.987,12.34,-23.45,34.56,45.67,-56.78,67.89,1013.25,23.45
```

### Python Analysis Script

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
df = pd.read_csv('data_20260503_143000.csv')

# Plot accelerometer data
plt.figure(figsize=(12, 6))
plt.plot(df['Timestamp_ms'], df['Accel_X_g'], label='X')
plt.plot(df['Timestamp_ms'], df['Accel_Y_g'], label='Y')
plt.plot(df['Timestamp_ms'], df['Accel_Z_g'], label='Z')
plt.xlabel('Time (ms)')
plt.ylabel('Acceleration (g)')
plt.legend()
plt.title('Accelerometer Data')
plt.show()
```

## Next Steps

After successful Phase A testing:
1. Proceed to Phase B (BLE Communication)
2. Integrate Android app development
3. Add WiFi communication (Phase C)

## Support

For issues or questions:
- Check ESP-IDF documentation: https://docs.espressif.com/projects/esp-idf/
- Review FSD documents: `Step1_PhaseA_FSD.md`
- Check component README files