# Phase B: BLE Communication Data Logger

## Overview

Phase B extends Phase A by adding Bluetooth Low Energy (BLE) communication capabilities. The ESP32-S3 firmware now includes a BLE GATT service that allows an Android app to:
- Start/stop data acquisition remotely
- Receive real-time sensor data
- Download stored data from SD card
- Monitor system status

## Key Features

### ESP32-S3 Firmware
- BLE GATT server with custom service
- Real-time data streaming over BLE
- Remote control via BLE commands
- Backward compatible with Phase A (local SD storage)
- Dual mode: BLE + SD card storage

### Android Application
- Compatible with Android 5.0.1+ (Samsung Galaxy S4 GT-I9500)
- BLE device scanning and connection
- Real-time data visualization
- Local data storage (internal or SD card)
- Start/stop acquisition control
- APK installation via USB or SD card (no Play Store)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-S3 Firmware                        │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │  Phase A   │  │  BLE GATT    │  │  BLE Data    │        │
│  │ Components │→ │   Service    │→ │  Streaming   │        │
│  └────────────┘  └──────────────┘  └──────────────┘        │
│                          ↓                                   │
└──────────────────────────┼───────────────────────────────────┘
                           │ BLE
                           ↓
┌─────────────────────────────────────────────────────────────┐
│                    Android Application                       │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │  BLE       │  │  Data        │  │  Local       │        │
│  │  Manager   │→ │  Handler     │→ │  Storage     │        │
│  └────────────┘  └──────────────┘  └──────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

## Project Structure

```
Step1_PhaseB_BLE/
├── CMakeLists.txt              # ESP-IDF project configuration
├── sdkconfig.defaults          # SDK configuration with BLE enabled
├── partitions.csv              # Flash partition table
├── README.md                   # This file
├── BUILD_AND_TEST.md          # Build and test guide
├── main/
│   ├── CMakeLists.txt         # Main component
│   ├── app_config.h           # Configuration (extends Phase A)
│   └── main.c                 # Application entry point
├── components/
│   ├── ble_service/           # BLE GATT service component
│   │   ├── CMakeLists.txt
│   │   ├── ble_service.h
│   │   └── ble_service.c
│   └── android_app/           # Android app source
│       ├── app/
│       │   ├── build.gradle
│       │   └── src/
│       │       └── main/
│       │           ├── AndroidManifest.xml
│       │           ├── java/
│       │           └── res/
│       ├── build.gradle
│       └── settings.gradle
└── docs/
    ├── BLE_PROTOCOL.md        # BLE communication protocol
    └── ANDROID_SETUP.md       # Android development setup
```

## BLE Service Specification

### Service UUID
- Service: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` (Nordic UART Service compatible)

### Characteristics

#### 1. TX Characteristic (Notify)
- UUID: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- Properties: Notify
- Purpose: Send sensor data to Android app

#### 2. RX Characteristic (Write)
- UUID: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- Properties: Write
- Purpose: Receive commands from Android app

#### 3. Status Characteristic (Read/Notify)
- UUID: `6E400004-B5A3-F393-E0A9-E50E24DCCA9E`
- Properties: Read, Notify
- Purpose: System status information

## BLE Commands

### From Android to ESP32-S3

| Command | Payload | Description |
|---------|---------|-------------|
| `START` | None | Start data acquisition |
| `STOP` | None | Stop data acquisition |
| `STATUS` | None | Request system status |
| `LIST` | None | List files on SD card |
| `DOWNLOAD` | filename | Download file from SD |
| `DELETE` | filename | Delete file from SD |
| `CLEAR` | None | Clear all buffers |

### From ESP32-S3 to Android

| Message Type | Format | Description |
|--------------|--------|-------------|
| Data | Binary | Sensor sample data |
| Status | JSON | System status |
| File List | JSON | SD card file list |
| File Data | Binary | File contents |
| ACK | Text | Command acknowledgment |
| ERROR | Text | Error message |

## Data Format

### Sensor Data Packet (Binary)
```
Byte 0-3:   Timestamp (uint32_t, ms)
Byte 4:     Module ID (uint8_t)
Byte 5:     Sensor Type (uint8_t)
Byte 6-9:   Accel X (float)
Byte 10-13: Accel Y (float)
Byte 14-17: Accel Z (float)
Byte 18-21: Gyro X (float)
Byte 22-25: Gyro Y (float)
Byte 26-29: Gyro Z (float)
Byte 30-33: Mag X (float)
Byte 34-37: Mag Y (float)
Byte 38-41: Mag Z (float)
Byte 42-45: Pressure (float)
Byte 46-49: Temperature (float)
Total: 50 bytes
```

### Status Message (JSON)
```json
{
  "state": "ACQUIRING",
  "battery": 85,
  "samples": 12345,
  "sd_free": 1024000000,
  "ble_connected": true
}
```

## Android App Features

### Main Screen
- BLE device scan and connect
- Connection status indicator
- Start/Stop acquisition button
- Real-time data display
- Battery level indicator

### Data View Screen
- Real-time sensor graphs
- Numeric data display
- Sample rate indicator
- Data statistics

### File Manager Screen
- List files on SD card
- Download files to phone
- Delete files
- Storage location selection

### Settings Screen
- BLE connection settings
- Data storage location
- Graph update rate
- Auto-reconnect option

## Development Requirements

### ESP32-S3 Firmware
- ESP-IDF v5.1+
- Bluetooth enabled in sdkconfig
- All Phase A components
- BLE service component

### Android App
- Android Studio Arctic Fox or later
- Minimum SDK: API 21 (Android 5.0.1)
- Target SDK: API 33 (Android 13)
- Gradle 7.0+
- Kotlin 1.7+

## Building

### ESP32-S3 Firmware
```bash
cd Step1_PhaseB_BLE
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

### Android App
```bash
cd Step1_PhaseB_BLE/components/android_app
./gradlew assembleDebug
# APK will be in app/build/outputs/apk/debug/
```

## Installation

### ESP32-S3
Flash firmware via USB as in Phase A.

### Android App
1. Copy APK to phone via USB or SD card
2. Enable "Install from Unknown Sources" in Settings
3. Open APK file and install
4. Grant Bluetooth and Storage permissions

## Testing

### BLE Connection Test
1. Flash ESP32-S3 firmware
2. Install Android app
3. Open app and scan for devices
4. Connect to "DataLogger_XXXX"
5. Verify connection status

### Data Streaming Test
1. Connect via BLE
2. Press "Start" in app
3. Verify real-time data display
4. Check sample rate
5. Press "Stop"

### File Transfer Test
1. Acquire data with SD storage
2. Connect via BLE
3. Open File Manager
4. Download file to phone
5. Verify file contents

## Compatibility

### Tested Devices
- Samsung Galaxy S4 GT-I9500 (Android 5.0.1)
- Samsung Galaxy S7 (Android 8.0)
- Google Pixel 3 (Android 11)

### Known Issues
- BLE range limited to ~10 meters
- Data rate limited by BLE throughput (~20KB/s)
- Older Android versions may have BLE stability issues

## Performance

### BLE Throughput
- Maximum: ~20 KB/s (160 kbps)
- Typical: ~15 KB/s (120 kbps)
- Latency: ~50ms

### Data Rate
- 1kHz sampling: 50 KB/s raw data
- BLE can handle ~300 Hz real-time streaming
- Higher rates require buffering and batch transfer

## Next Steps

After Phase B completion:
1. Proceed to Phase C (WiFi communication)
2. Add real-time visualization
3. Implement WiFi Direct
4. Enhance Android app features

## References

- Phase A Implementation: `../Step1_PhaseA_DataLogger/`
- Phase B FSD: `../Step1_PhaseB_FSD.md`
- BLE Protocol: `docs/BLE_PROTOCOL.md`
- Android Setup: `docs/ANDROID_SETUP.md`