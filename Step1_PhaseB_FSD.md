# Functional Specification Document (FSD)
# Phase B: BLE Communication
# IoT Multi-Sensor Data Logger

**Project Name**: IoT Multi-Sensor Data Logger - Phase B  
**Hardware Platform**: Unexpected Maker ProS3 (ESP32-S3)  
**Phase**: B - BLE Communication  
**Document Version**: 1.0  
**Date**: 2026-05-03  
**Author**: Bob (Advanced Mode)  
**Status**: Ready for Implementation  
**Prerequisites**: Phase A Complete

---

## Table of Contents

1. [Phase B Overview](#1-phase-b-overview)
2. [Requirements](#2-requirements)
3. [BLE Architecture](#3-ble-architecture)
4. [Android App Architecture](#4-android-app-architecture)
5. [Implementation Details](#5-implementation-details)
6. [Testing & Validation](#6-testing--validation)
7. [Project Structure](#7-project-structure)
8. [Implementation Timeline](#8-implementation-timeline)

---

## 1. Phase B Overview

### 1.1 Objectives
Phase B adds Bluetooth Low Energy (BLE) wireless communication capabilities to the data logger, enabling:
- Wireless data streaming to Android devices
- Remote start/stop control
- Real-time status monitoring
- Data storage on mobile device
- Android 5.0.1+ compatibility

### 1.2 Success Criteria
- [ ] BLE connection established in <3 seconds
- [ ] Data throughput >50KB/s
- [ ] BLE range >10 meters
- [ ] Connection stability >99% over 1 hour
- [ ] Android app works on Android 5.0.1+
- [ ] APK installs without Play Store
- [ ] Phase A functionality maintained

### 1.3 Deliverables
1. BLE-enabled ESP32 firmware
2. Android application (Java, API 21+)
3. BLE communication protocol documentation
4. Android app APK file
5. User manual for mobile app
6. Test results and validation report

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Acceptance Criteria |
|----|-------------|----------|---------------------|
| PB-FR-001 | ESP32 shall advertise BLE service | CRITICAL | Visible in BLE scanner |
| PB-FR-002 | ESP32 shall accept BLE connections | CRITICAL | Connection successful |
| PB-FR-003 | ESP32 shall stream sensor data via BLE | CRITICAL | Data received on phone |
| PB-FR-004 | ESP32 shall accept start/stop commands | HIGH | Remote control works |
| PB-FR-005 | Android app shall discover devices | CRITICAL | Device list populated |
| PB-FR-006 | Android app shall display real-time status | HIGH | UI updates <100ms |
| PB-FR-007 | Android app shall store data locally | CRITICAL | CSV files created |
| PB-FR-008 | Android app shall support Android 5.0.1+ | CRITICAL | Tested on API 21 device |
| PB-FR-009 | System shall maintain Phase A modes | HIGH | DIP switches still work |
| PB-FR-010 | App shall install via APK | MEDIUM | Side-loading successful |

### 2.2 Performance Requirements

| ID | Requirement | Target | Measurement Method |
|----|-------------|--------|-------------------|
| PB-PR-001 | BLE connection time | <3 seconds | Stopwatch |
| PB-PR-002 | BLE throughput | >50KB/s | Benchmark test |
| PB-PR-003 | BLE range | >10 meters | Distance test |
| PB-PR-004 | Connection stability | >99% uptime | 1-hour test |
| PB-PR-005 | Battery life (BLE mode) | >50 minutes | Discharge test |
| PB-PR-006 | App responsiveness | <100ms UI update | Profiler |
| PB-PR-007 | Data loss rate | <0.1% | Packet analysis |

### 2.3 Android Compatibility

| Android Version | API Level | Support Status | Test Device |
|-----------------|-----------|----------------|-------------|
| 5.0.1 (Lollipop) | 21 | Required | Samsung Galaxy S4 |
| 6.0 (Marshmallow) | 23 | Supported | Generic |
| 7.0 (Nougat) | 24 | Supported | Generic |
| 8.0 (Oreo) | 26 | Supported | Generic |
| 9.0 (Pie) | 28 | Supported | Generic |
| 10+ | 29+ | Supported | Modern devices |

---

## 3. BLE Architecture

### 3.1 BLE Service Definition

```c
// Service UUID (128-bit custom)
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

// Characteristics
#define CHAR_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write
#define CHAR_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Notify
#define CHAR_STATUS_UUID    "6E400004-B5A3-F393-E0A9-E50E24DCCA9E"  // Read

// Service structure
typedef struct {
    uint16_t service_handle;
    uint16_t rx_char_handle;
    uint16_t tx_char_handle;
    uint16_t status_char_handle;
    uint16_t conn_id;
    bool connected;
} ble_service_t;
```

### 3.2 BLE Characteristics

#### RX Characteristic (Commands from App)
```c
// Write-only, receives commands from Android app
typedef enum {
    CMD_START_LOG = 0x01,       // Start data logging
    CMD_STOP_LOG = 0x02,        // Stop data logging
    CMD_GET_STATUS = 0x03,      // Request status update
    CMD_SET_SAMPLE_RATE = 0x04, // Change sample rate
    CMD_CALIBRATE = 0x05,       // Start calibration
    CMD_GET_BATTERY = 0x06,     // Request battery status
    CMD_RESET = 0x07,           // Reset system
    CMD_GET_INFO = 0x08         // Get device info
} ble_command_t;

// Command packet structure
typedef struct {
    uint8_t command;            // Command code
    uint8_t param_len;          // Parameter length
    uint8_t params[32];         // Parameters (if any)
} __attribute__((packed)) ble_cmd_packet_t;
```

#### TX Characteristic (Data to App)
```c
// Notify-only, sends sensor data to Android app
// Sends sensor_sample_t structures (72 bytes per packet)
// Uses BLE notifications for efficient streaming

// Data packet structure
typedef struct {
    uint16_t magic;             // 0xAA55
    uint8_t type;               // 0x01=Data, 0x02=Status, 0x03=Error
    uint8_t length;             // Payload length
    uint8_t payload[244];       // Payload (max MTU-4)
    uint16_t crc;               // CRC16 checksum
} __attribute__((packed)) ble_tx_packet_t;
```

#### Status Characteristic
```c
// Read-only, provides system status
typedef struct {
    uint8_t state;              // Current system state
    uint16_t battery_mv;        // Battery voltage (mV)
    uint32_t samples_logged;    // Total samples logged
    uint32_t uptime_sec;        // Uptime in seconds
    uint8_t error_code;         // Last error code
    uint8_t sd_status;          // SD card status
    uint32_t free_space_mb;     // Free space on SD (MB)
} __attribute__((packed)) ble_status_t;
```

### 3.3 BLE Connection Parameters

```c
// Connection parameters
#define BLE_CONN_INTERVAL_MIN   6       // 7.5ms (6 * 1.25ms)
#define BLE_CONN_INTERVAL_MAX   12      // 15ms (12 * 1.25ms)
#define BLE_CONN_LATENCY        0       // No latency
#define BLE_CONN_TIMEOUT        400     // 4s (400 * 10ms)

// MTU negotiation
#define BLE_MTU_SIZE            247     // Request 247 bytes MTU
#define BLE_MAX_PAYLOAD         (BLE_MTU_SIZE - 3)  // 244 bytes

// Advertising parameters
#define BLE_ADV_INTERVAL_MIN    0x20    // 20ms
#define BLE_ADV_INTERVAL_MAX    0x40    // 40ms
#define BLE_ADV_TIMEOUT         0       // No timeout
```

### 3.4 BLE State Machine

```c
typedef enum {
    BLE_STATE_IDLE,             // Not advertising
    BLE_STATE_ADVERTISING,      // Advertising, waiting for connection
    BLE_STATE_CONNECTED,        // Connected to device
    BLE_STATE_STREAMING,        // Actively streaming data
    BLE_STATE_ERROR             // Error state
} ble_state_t;

typedef enum {
    BLE_EVENT_START_ADV,        // Start advertising
    BLE_EVENT_STOP_ADV,         // Stop advertising
    BLE_EVENT_CONNECTED,        // Device connected
    BLE_EVENT_DISCONNECTED,     // Device disconnected
    BLE_EVENT_START_STREAM,     // Start data streaming
    BLE_EVENT_STOP_STREAM,      // Stop data streaming
    BLE_EVENT_ERROR             // Error occurred
} ble_event_t;
```

---

## 4. Android App Architecture

### 4.1 App Structure

```
Android App Architecture:

┌─────────────────────────────────────────────────┐
│         Presentation Layer                      │
│  - MainActivity (device scan, connection)       │
│  - StatusActivity (real-time status)            │
│  - DataActivity (data logging control)          │
│  - SettingsActivity (app configuration)         │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│         Business Logic Layer                    │
│  - BLEManager (BLE operations)                  │
│  - DataLogger (file storage)                    │
│  - DataParser (sensor data parsing)             │
│  - CommandHandler (send commands)               │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│         Data Layer                              │
│  - FileStorage (CSV writer)                     │
│  - SharedPreferences (settings)                 │
│  - Database (optional: SQLite for history)      │
└─────────────────────────────────────────────────┘
```

### 4.2 Android App Components

#### MainActivity
```java
public class MainActivity extends AppCompatActivity {
    // BLE scanning and device selection
    private BluetoothAdapter bluetoothAdapter;
    private BLEManager bleManager;
    private RecyclerView deviceListView;
    
    // Scan for BLE devices
    private void startScan();
    
    // Connect to selected device
    private void connectToDevice(BluetoothDevice device);
    
    // Navigate to status screen
    private void openStatusActivity();
}
```

#### StatusActivity
```java
public class StatusActivity extends AppCompatActivity {
    // Real-time status display
    private TextView batteryText;
    private TextView stateText;
    private TextView samplesText;
    private TextView uptimeText;
    
    // Update UI with status
    private void updateStatus(BLEStatus status);
    
    // Start/stop logging
    private void toggleLogging();
}
```

#### BLEManager
```java
public class BLEManager {
    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic rxChar;
    private BluetoothGattCharacteristic txChar;
    private BluetoothGattCharacteristic statusChar;
    
    // Connect to device
    public void connect(BluetoothDevice device);
    
    // Send command
    public void sendCommand(byte command, byte[] params);
    
    // Register data callback
    public void setDataCallback(DataCallback callback);
    
    // Disconnect
    public void disconnect();
}
```

#### DataLogger
```java
public class DataLogger {
    private File logFile;
    private BufferedWriter writer;
    
    // Create new log file
    public void createLogFile(String filename);
    
    // Write sensor sample
    public void writeSample(SensorSample sample);
    
    // Close log file
    public void closeLogFile();
    
    // Get storage location
    public File getStorageDirectory();
}
```

### 4.3 Android Permissions

```xml
<!-- AndroidManifest.xml -->
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />

<!-- For Android 12+ -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
```

### 4.4 Data Storage Locations

```
Android Storage Options:

1. Internal Storage (Primary):
   /data/data/com.example.datalogger/files/
   - Private to app
   - Always available
   - Limited space

2. External Storage (SD Card):
   /storage/emulated/0/DataLogger/
   or
   /storage/sdcard1/DataLogger/
   - User accessible
   - More space
   - May be removable

3. Downloads Folder:
   /storage/emulated/0/Download/DataLogger/
   - Easy user access
   - Standard location
```

---

## 5. Implementation Details

### 5.1 ESP32 BLE Service Implementation

**File**: `ble_service.c/h`

```c
// Initialize BLE service
esp_err_t ble_service_init(void);

// Start advertising
esp_err_t ble_service_start_advertising(void);

// Stop advertising
esp_err_t ble_service_stop_advertising(void);

// Send data packet
esp_err_t ble_service_send_data(sensor_sample_t* sample);

// Send status update
esp_err_t ble_service_send_status(ble_status_t* status);

// Register command callback
typedef void (*ble_cmd_callback_t)(ble_command_t cmd, uint8_t* params, uint8_t len);
esp_err_t ble_service_register_cmd_callback(ble_cmd_callback_t callback);

// Get connection status
bool ble_service_is_connected(void);

// Get BLE state
ble_state_t ble_service_get_state(void);
```

### 5.2 Android BLE Manager Implementation

**File**: `BLEManager.java`

```java
public class BLEManager {
    private static final String SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    private static final String RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    private static final String TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    private static final String STATUS_CHAR_UUID = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";
    
    // Scan for devices
    public void startScan(ScanCallback callback);
    
    // Connect to device
    public void connect(BluetoothDevice device, ConnectionCallback callback);
    
    // Send command
    public void sendCommand(byte command, byte[] params);
    
    // Enable notifications
    public void enableNotifications();
    
    // Data callback interface
    public interface DataCallback {
        void onDataReceived(SensorSample sample);
        void onStatusReceived(BLEStatus status);
        void onError(String error);
    }
}
```

### 5.3 Data Packet Format

```c
// BLE TX Packet Format
// Total size: 250 bytes (fits in 247 MTU + overhead)

typedef struct {
    // Header (6 bytes)
    uint16_t magic;             // 0xAA55
    uint8_t type;               // Packet type
    uint8_t length;             // Payload length
    uint16_t sequence;          // Sequence number
    
    // Payload (240 bytes max)
    union {
        sensor_sample_t samples[3];  // 3 samples (72 bytes each = 216 bytes)
        ble_status_t status;         // Status (12 bytes)
        uint8_t raw[240];            // Raw data
    } payload;
    
    // Footer (4 bytes)
    uint16_t crc;               // CRC16 checksum
    uint16_t reserved;          // Reserved for future use
} __attribute__((packed)) ble_packet_t;

// Packet types
#define PKT_TYPE_DATA       0x01
#define PKT_TYPE_STATUS     0x02
#define PKT_TYPE_ERROR      0x03
#define PKT_TYPE_ACK        0x04
```

### 5.4 CSV File Format (Android)

```csv
# IoT Sensor Data Logger - Phase B
# Android App Version: 1.0.0
# Device: ESP32_DataLogger_XXXX
# Start Time: 2026-05-03T14:30:22Z
# Sample Rate: 1000 Hz
# Module ID: 1
Timestamp_us,Accel_X_mg,Accel_Y_mg,Accel_Z_mg,Gyro_X_mdps,Gyro_Y_mdps,Gyro_Z_mdps,Mag_X_uT,Mag_Y_uT,Mag_Z_uT,Pressure_hPa,Temp_C
0,5,-3,1024,12,-8,5,45.2,-12.3,38.7,1013.25,22.5
```

---

## 6. Testing & Validation

### 6.1 BLE Communication Tests

| Test ID | Description | Pass Criteria |
|---------|-------------|---------------|
| BT-001 | BLE advertising | Device visible in scanner |
| BT-002 | Connection establishment | Connection in <3s |
| BT-003 | MTU negotiation | MTU = 247 bytes |
| BT-004 | Data streaming | >50KB/s throughput |
| BT-005 | Command handling | All commands work |
| BT-006 | Disconnection handling | Graceful disconnect |
| BT-007 | Reconnection | Auto-reconnect works |
| BT-008 | Range test | >10m range |
| BT-009 | Stability test | >99% uptime in 1hr |
| BT-010 | Multiple connections | Reject 2nd connection |

### 6.2 Android App Tests

| Test ID | Description | Pass Criteria |
|---------|-------------|---------------|
| AT-001 | Device scanning | Finds ESP32 device |
| AT-002 | Connection | Connects successfully |
| AT-003 | Data reception | Receives sensor data |
| AT-004 | File creation | CSV file created |
| AT-005 | Data logging | Data written correctly |
| AT-006 | Start/stop control | Remote control works |
| AT-007 | Status display | UI updates correctly |
| AT-008 | Battery display | Shows battery level |
| AT-009 | Error handling | Errors displayed |
| AT-010 | APK installation | Installs via USB/SD |

### 6.3 Compatibility Tests

| Device | Android Version | API Level | Test Result |
|--------|-----------------|-----------|-------------|
| Samsung Galaxy S4 | 5.0.1 | 21 | To test |
| Generic Phone | 6.0 | 23 | To test |
| Generic Phone | 7.0 | 24 | To test |
| Generic Phone | 8.0 | 26 | To test |
| Modern Phone | 10+ | 29+ | To test |

---

## 7. Project Structure

### 7.1 ESP32 Firmware Structure

```
Step1_PhaseB_DataLogger/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── README.md
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   └── app_config.h
├── components/
│   ├── ble_service/
│   │   ├── CMakeLists.txt
│   │   ├── ble_service.c
│   │   ├── ble_service.h
│   │   └── ble_protocol.h
│   ├── [Phase A components...]
│   └── comm_manager/
│       ├── CMakeLists.txt
│       ├── comm_manager.c
│       └── comm_manager.h
└── test/
    └── test_ble.c
```

### 7.2 Android App Structure

```
DataLoggerApp/
├── app/
│   ├── build.gradle
│   ├── src/
│   │   ├── main/
│   │   │   ├── AndroidManifest.xml
│   │   │   ├── java/com/example/datalogger/
│   │   │   │   ├── MainActivity.java
│   │   │   │   ├── StatusActivity.java
│   │   │   │   ├── DataActivity.java
│   │   │   │   ├── SettingsActivity.java
│   │   │   │   ├── ble/
│   │   │   │   │   ├── BLEManager.java
│   │   │   │   │   ├── BLEDevice.java
│   │   │   │   │   └── BLECallback.java
│   │   │   │   ├── data/
│   │   │   │   │   ├── DataLogger.java
│   │   │   │   │   ├── DataParser.java
│   │   │   │   │   └── SensorSample.java
│   │   │   │   └── utils/
│   │   │   │       ├── FileUtils.java
│   │   │   │       └── PermissionUtils.java
│   │   │   └── res/
│   │   │       ├── layout/
│   │   │       │   ├── activity_main.xml
│   │   │       │   ├── activity_status.xml
│   │   │       │   └── activity_data.xml
│   │   │       ├── values/
│   │   │       │   ├── strings.xml
│   │   │       │   └── colors.xml
│   │   │       └── drawable/
│   │   └── test/
│   │       └── java/com/example/datalogger/
│   │           └── BLEManagerTest.java
├── build.gradle
└── settings.gradle
```

---

## 8. Implementation Timeline

### Week 9: BLE Stack Setup
**Days 1-2**: ESP-IDF BLE Configuration
- [ ] Enable BLE in sdkconfig
- [ ] Configure BLE stack parameters
- [ ] Initialize BLE controller
- [ ] Test basic BLE functionality

**Days 3-4**: BLE Service Implementation
- [ ] Define service and characteristics
- [ ] Implement GATT server
- [ ] Implement advertising
- [ ] Test with generic BLE scanner

**Days 5-7**: BLE Integration
- [ ] Integrate with Phase A code
- [ ] Add BLE mode to state machine
- [ ] Test mode switching
- [ ] Verify Phase A still works

### Week 10: BLE Data Transfer
**Days 1-2**: TX Characteristic
- [ ] Implement data packet format
- [ ] Implement notification sending
- [ ] Test data streaming
- [ ] Measure throughput

**Days 3-4**: RX Characteristic
- [ ] Implement command reception
- [ ] Implement command parsing
- [ ] Implement command handlers
- [ ] Test all commands

**Days 5-7**: Optimization
- [ ] Optimize packet size
- [ ] Implement MTU negotiation
- [ ] Test connection parameters
- [ ] Performance tuning

### Week 11: Android App Foundation
**Days 1-2**: Project Setup
- [ ] Create Android Studio project (API 21)
- [ ] Setup dependencies
- [ ] Configure permissions
- [ ] Create basic UI layouts

**Days 3-4**: BLE Scanning
- [ ] Implement BLE scanner
- [ ] Implement device list
- [ ] Test on Android 5.0.1 device
- [ ] Handle permissions

**Days 5-7**: BLE Connection
- [ ] Implement connection logic
- [ ] Implement service discovery
- [ ] Enable notifications
- [ ] Test connection stability

### Week 12: Android Data Handling
**Days 1-2**: Data Reception
- [ ] Implement data callback
- [ ] Parse sensor packets
- [ ] Display real-time data
- [ ] Test data integrity

**Days 3-4**: File Storage
- [ ] Implement CSV writer
- [ ] Handle storage permissions
- [ ] Test SD card storage
- [ ] Test internal storage

**Days 5-7**: Control Features
- [ ] Implement start/stop buttons
- [ ] Implement status display
- [ ] Implement battery display
- [ ] Test remote control

### Week 13: Integration Testing
**Days 1-2**: End-to-End Testing
- [ ] Test complete workflow
- [ ] Test on multiple devices
- [ ] Test different Android versions
- [ ] Fix bugs

**Days 3-4**: Performance Testing
- [ ] Measure throughput
- [ ] Test range
- [ ] Test stability
- [ ] Test battery life

**Days 5-7**: Compatibility Testing
- [ ] Test on Android 5.0.1
- [ ] Test on Android 6.0+
- [ ] Test on different phones
- [ ] Document issues

### Week 14: Documentation & Deployment
**Days 1-2**: Documentation
- [ ] Write user manual
- [ ] Write API documentation
- [ ] Create troubleshooting guide
- [ ] Record demo video

**Days 3-4**: APK Build
- [ ] Build release APK
- [ ] Sign APK
- [ ] Test installation via USB
- [ ] Test installation via SD card

**Days 5-7**: Final Validation
- [ ] Complete system test
- [ ] User acceptance test
- [ ] Phase B sign-off
- [ ] Prepare for Phase C

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-05-03 | Bob (Advanced Mode) | Initial Phase B FSD |

---

**END OF PHASE B FSD**