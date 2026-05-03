# ESP32 Sensor Logger - Android App (Phase B)

BLE-enabled Android application for the ESP32 Sensor Logger project. This app communicates with the ESP32-S3 device via Bluetooth Low Energy (BLE) to control data acquisition and store sensor data locally on the Android device.

## Features

### Phase B Capabilities
- **BLE Device Scanning**: Discover and connect to ESP32-S3 devices advertising the Nordic UART Service (NUS)
- **Remote Control**: Start/stop data acquisition on the ESP32 via BLE commands
- **Real-time Data Display**: View live sensor readings from all three sensors:
  - ICM-42688-P: Accelerometer (±16g) and Gyroscope (±2000°/s)
  - MMC5983MA: Magnetometer
  - LPS22HB: Pressure and Temperature
- **Device Status Monitoring**: Battery level, sample rate, buffer usage, and system state
- **Local Data Storage**: Save received sensor data to CSV files on phone storage or SD card
- **Storage Location Selection**: Choose between internal storage, primary external storage, or SD card
- **Data Viewer**: Browse and manage saved CSV files

## System Requirements

### Minimum Requirements
- **Android Version**: 5.0.1 (API 21) - Lollipop
- **Bluetooth**: BLE (Bluetooth Low Energy) support required
- **Storage**: 100 MB free space recommended
- **Permissions**: Bluetooth, Location (for BLE scanning), Storage

### Tested Devices
- Samsung Galaxy S4 GT-I9500 (Android 5.0.1)
- Modern Android devices (Android 10+)

## Installation

### Method 1: Direct APK Installation (Recommended)
1. Download the APK file to your Android device
2. Enable "Install from Unknown Sources" in Settings → Security
3. Open the APK file and follow installation prompts
4. Grant required permissions when prompted

### Method 2: SD Card Installation
1. Copy the APK file to your device's SD card
2. Use a file manager app to navigate to the APK
3. Tap the APK to install
4. Grant required permissions

### Method 3: USB Installation (ADB)
```bash
adb install -r app-release.apk
```

## Building from Source

### Prerequisites
- Android Studio Arctic Fox (2020.3.1) or later
- Android SDK API 33
- Gradle 8.1.0
- Kotlin 1.8.22

### Build Steps
1. Clone the repository
2. Open the project in Android Studio
3. Sync Gradle files
4. Build the APK:
   ```bash
   ./gradlew assembleRelease
   ```
5. APK location: `app/build/outputs/apk/release/app-release.apk`

## Usage Guide

### 1. Initial Setup
1. Launch the app
2. Grant Bluetooth and Storage permissions when prompted
3. Enable Bluetooth if not already enabled

### 2. Connecting to ESP32
1. Tap "Connect to Device" button
2. The app will scan for nearby ESP32 devices
3. Select your ESP32-S3 device from the list
4. Wait for connection confirmation

### 3. Data Acquisition
1. Select storage location (Internal/External/SD Card)
2. Tap "Start Acquisition" to begin recording
3. Monitor real-time sensor data on screen
4. Tap "Stop Acquisition" to end recording
5. Data is automatically saved to CSV file

### 4. Viewing Saved Data
1. Tap "View Saved Data" button
2. Browse list of CSV files
3. Tap a file to view details
4. Options: Share, Delete, or Export

### 5. Device Status
Monitor ESP32 status in real-time:
- **State**: IDLE, ACQUIRING, STORING, ERROR
- **Battery**: Voltage and percentage
- **Sample Rate**: Current sampling frequency (Hz)
- **Buffer Usage**: ESP32 buffer fill percentage

## BLE Communication Protocol

### Nordic UART Service (NUS)
- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic** (Write): `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX Characteristic** (Notify): `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- **Status Characteristic** (Read): `6E400004-B5A3-F393-E0A9-E50E24DCCA9E`

### Commands (RX Characteristic)
- `START`: Begin data acquisition
- `STOP`: Stop data acquisition
- `STATUS`: Request device status update

### Data Format (TX Characteristic)
Binary format (52 bytes per sample):
```
Offset | Size | Field
-------|------|------------------
0      | 8    | Timestamp (ms)
8      | 4    | Accel X (float)
12     | 4    | Accel Y (float)
16     | 4    | Accel Z (float)
20     | 4    | Gyro X (float)
24     | 4    | Gyro Y (float)
28     | 4    | Gyro Z (float)
32     | 4    | Mag X (float)
36     | 4    | Mag Y (float)
40     | 4    | Mag Z (float)
44     | 4    | Pressure (float)
48     | 4    | Temperature (float)
```

All multi-byte values are little-endian.

### Status Format (Status Characteristic)
Binary format (9 bytes):
```
Offset | Size | Field
-------|------|------------------
0      | 1    | State (0=IDLE, 1=ACQUIRING, 2=STORING, 3=ERROR)
1      | 4    | Battery Voltage (float)
5      | 1    | Battery Percent (uint8)
6      | 2    | Sample Rate (uint16)
8      | 1    | Buffer Usage % (uint8)
```

## File Storage

### CSV File Format
```csv
timestamp,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,pressure,temperature
1234567890,0.123,-0.456,9.812,1.23,-4.56,7.89,12.34,-56.78,90.12,1013.25,23.45
...
```

### Storage Locations
1. **Internal Storage**: `/data/data/com.esp32.sensorlogger/files/sensor_data/`
2. **Primary External**: `/storage/emulated/0/SensorLogger/`
3. **SD Card**: `/storage/sdcard1/SensorLogger/`

### File Naming
Format: `sensor_data_YYYYMMDD_HHMMSS.csv`
Example: `sensor_data_20260503_143022.csv`

## Permissions

### Required Permissions
- `BLUETOOTH` / `BLUETOOTH_ADMIN` (Android < 12)
- `BLUETOOTH_SCAN` / `BLUETOOTH_CONNECT` (Android 12+)
- `ACCESS_FINE_LOCATION` (Required for BLE scanning)
- `WRITE_EXTERNAL_STORAGE` / `READ_EXTERNAL_STORAGE` (Android < 10)

## Troubleshooting

### Connection Issues
- **Problem**: Cannot find ESP32 device
  - **Solution**: Ensure ESP32 is powered on and BLE is enabled
  - **Solution**: Check that device is advertising NUS service
  - **Solution**: Move closer to the device (BLE range ~10m)

- **Problem**: Connection drops frequently
  - **Solution**: Reduce distance between phone and ESP32
  - **Solution**: Avoid obstacles and interference sources
  - **Solution**: Check ESP32 battery level

### Data Issues
- **Problem**: No data received
  - **Solution**: Verify acquisition is started on ESP32
  - **Solution**: Check BLE notifications are enabled
  - **Solution**: Restart both app and ESP32

- **Problem**: File creation fails
  - **Solution**: Check storage permissions
  - **Solution**: Ensure sufficient free space
  - **Solution**: Try different storage location

### Performance Issues
- **Problem**: App crashes or freezes
  - **Solution**: Clear app cache and data
  - **Solution**: Reinstall the app
  - **Solution**: Check Android version compatibility

## Architecture

### Key Components
- **BleManager**: Handles BLE scanning, connection, and communication
- **CsvFileManager**: Manages CSV file creation, writing, and storage
- **MainActivity**: Main UI with real-time data display
- **BleScanActivity**: BLE device scanning and selection
- **DataViewerActivity**: Browse and manage saved files
- **MainViewModel**: Business logic and state management

### Dependencies
- AndroidX Core, AppCompat, Material Design
- Lifecycle components (ViewModel, LiveData)
- Kotlin Coroutines
- Nordic BLE Library
- Dexter (Permissions)
- OpenCSV
- Timber (Logging)

## Known Limitations

### Phase B Limitations
1. **BLE Throughput**: Maximum ~20 KB/s (vs 50 KB/s sensor data rate)
   - Effective streaming rate: ~300 Hz (vs 1000 Hz acquisition)
   - Data is buffered on ESP32 and transmitted when possible
2. **No Real-time Visualization**: Charts will be added in Phase C
3. **WiFi Not Supported**: WiFi communication added in Phase C
4. **Single Device**: Can only connect to one ESP32 at a time

### Android 5.0.1 Limitations
- Older BLE stack may have reduced performance
- Some Material Design features may render differently
- Limited background processing capabilities

## Future Enhancements (Phase C)

Phase C will add:
- **WiFi Communication**: HTTP REST API and WebSocket streaming
- **Real-time Charts**: Live visualization of sensor data
- **WiFi Direct**: Phone as access point for ESP32
- **Enhanced Performance**: Higher data rates via WiFi
- **Web Dashboard**: Browser-based monitoring interface

## Support

For issues, questions, or contributions:
- Check the main project documentation
- Review the ESP32 firmware README
- Consult the Phase B FSD document

## License

This project is part of the ESP32 Sensor Logger system.
See main project LICENSE file for details.

## Version History

### v1.0.0-phaseB (Current)
- Initial Phase B release
- BLE communication with ESP32-S3
- Remote acquisition control
- Local CSV storage
- Real-time data display
- Device status monitoring
- Multi-location storage support
- Android 5.0.1+ compatibility