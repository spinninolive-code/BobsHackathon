# Functional Specification Document (FSD)
# Phase C: WiFi Communication & Visualization
# IoT Multi-Sensor Data Logger

**Project Name**: IoT Multi-Sensor Data Logger - Phase C  
**Hardware Platform**: Unexpected Maker ProS3 (ESP32-S3)  
**Phase**: C - WiFi & Visualization  
**Document Version**: 1.0  
**Date**: 2026-05-03  
**Author**: Bob (Advanced Mode)  
**Status**: Ready for Implementation  
**Prerequisites**: Phase A & B Complete

---

## Table of Contents

1. [Phase C Overview](#1-phase-c-overview)
2. [Requirements](#2-requirements)
3. [WiFi Architecture](#3-wifi-architecture)
4. [Android App Enhancements](#4-android-app-enhancements)
5. [Data Visualization](#5-data-visualization)
6. [Implementation Details](#6-implementation-details)
7. [Testing & Validation](#7-testing--validation)
8. [Project Structure](#8-project-structure)
9. [Implementation Timeline](#9-implementation-timeline)

---

## 1. Phase C Overview

### 1.1 Objectives
Phase C adds WiFi communication and real-time data visualization capabilities:
- High-speed WiFi data streaming (>100KB/s)
- Android phone as WiFi Access Point
- ESP32 as WiFi client
- Real-time chart visualization
- Enhanced data analysis features
- WiFi Direct support (optional)

### 1.2 Success Criteria
- [ ] WiFi connection established in <5 seconds
- [ ] Data throughput >100KB/s
- [ ] WiFi range >20 meters
- [ ] Chart update rate >10 Hz
- [ ] Battery life >35 minutes (WiFi mode)
- [ ] Latency <200ms (sensor to display)
- [ ] BLE fallback functional
- [ ] All Phase A & B features maintained

### 1.3 Deliverables
1. WiFi-enabled ESP32 firmware
2. Enhanced Android application with charts
3. WiFi communication protocol documentation
4. Real-time visualization system
5. Updated APK file
6. Complete user manual
7. Final test results and validation report

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Acceptance Criteria |
|----|-------------|----------|---------------------|
| PC-FR-001 | ESP32 shall create WiFi AP or join network | CRITICAL | WiFi connection successful |
| PC-FR-002 | ESP32 shall stream data via WiFi | CRITICAL | Data received on phone |
| PC-FR-003 | Android phone shall act as WiFi AP | HIGH | Hotspot created |
| PC-FR-004 | ESP32 shall connect to phone's WiFi | CRITICAL | Connection successful |
| PC-FR-005 | Android app shall visualize data in real-time | HIGH | Charts update smoothly |
| PC-FR-006 | App shall display charts for all sensor axes | HIGH | All 10 DOF visible |
| PC-FR-007 | System shall support WiFi Direct | MEDIUM | P2P connection works |
| PC-FR-008 | System shall maintain BLE fallback | HIGH | BLE still functional |
| PC-FR-009 | App shall allow WiFi configuration | MEDIUM | SSID/password editable |
| PC-FR-010 | System shall handle WiFi disconnections | HIGH | Auto-reconnect works |

### 2.2 Performance Requirements

| ID | Requirement | Target | Measurement Method |
|----|-------------|--------|-------------------|
| PC-PR-001 | WiFi connection time | <5 seconds | Stopwatch |
| PC-PR-002 | WiFi throughput | >100KB/s | Benchmark test |
| PC-PR-003 | WiFi range | >20 meters | Distance test |
| PC-PR-004 | Chart update rate | >10 Hz | Frame counter |
| PC-PR-005 | Battery life (WiFi mode) | >35 minutes | Discharge test |
| PC-PR-006 | Latency (sensor to display) | <200ms | Timestamp analysis |
| PC-PR-007 | Data loss rate | <0.1% | Packet analysis |
| PC-PR-008 | CPU utilization (ESP32) | <85% | Profiler |

### 2.3 Visualization Requirements

| ID | Requirement | Description |
|----|-------------|-------------|
| PC-VR-001 | Real-time charts | Update at >10Hz |
| PC-VR-002 | Multi-axis display | Show all 10 DOF simultaneously |
| PC-VR-003 | Zoom and pan | User can zoom/pan charts |
| PC-VR-004 | Time window | Configurable display window (1s-60s) |
| PC-VR-005 | Auto-scale | Y-axis auto-scales to data |
| PC-VR-006 | Color coding | Different colors per axis |
| PC-VR-007 | Legend | Clear axis labels |
| PC-VR-008 | Export | Save chart as image |

---

## 3. WiFi Architecture

### 3.1 WiFi Modes

```c
// WiFi operating modes
typedef enum {
    WIFI_MODE_OFF,          // WiFi disabled
    WIFI_MODE_STA,          // Station mode (client)
    WIFI_MODE_AP,           // Access Point mode
    WIFI_MODE_APSTA         // Both (WiFi Direct)
} wifi_mode_t;

// WiFi configuration
typedef struct {
    char ssid[32];          // Network SSID
    char password[64];      // Network password
    wifi_mode_t mode;       // Operating mode
    uint8_t channel;        // WiFi channel (1-13)
    uint8_t max_connections;// Max clients (AP mode)
    bool hidden;            // Hidden SSID (AP mode)
} wifi_config_t;
```

### 3.2 WiFi Connection Scenarios

#### Scenario 1: Phone as AP (Primary)
```
Android Phone (AP)          ESP32 (STA)
192.168.43.1:8080    ←→    192.168.43.xxx

1. User enables hotspot on phone
2. ESP32 scans and finds phone's SSID
3. ESP32 connects to phone's AP
4. TCP connection established
5. Data streaming begins
```

#### Scenario 2: ESP32 as AP (Alternative)
```
ESP32 (AP)                  Android Phone (STA)
192.168.4.1:8080     ←→    192.168.4.xxx

1. ESP32 creates AP
2. User connects phone to ESP32's AP
3. TCP connection established
4. Data streaming begins
```

#### Scenario 3: WiFi Direct (Optional)
```
Android Phone (P2P GO)      ESP32 (P2P Client)
192.168.49.1:8080    ←→    192.168.49.xxx

1. Phone initiates WiFi Direct
2. ESP32 accepts P2P invitation
3. Direct connection established
4. Data streaming begins
```

### 3.3 TCP Communication Protocol

```c
// TCP Server Configuration
#define TCP_SERVER_PORT     8080
#define TCP_MAX_CLIENTS     1
#define TCP_KEEPALIVE       true
#define TCP_KEEPIDLE        5       // seconds
#define TCP_KEEPINTVL       5       // seconds
#define TCP_KEEPCNT         3       // count

// Message format (same as BLE for consistency)
typedef struct {
    uint16_t magic;             // 0xAA55
    uint8_t type;               // Message type
    uint8_t length;             // Payload length
    uint16_t sequence;          // Sequence number
    uint8_t payload[1024];      // Payload (larger than BLE)
    uint16_t crc;               // CRC16 checksum
} __attribute__((packed)) tcp_packet_t;

// Message types
#define MSG_TYPE_DATA       0x01    // Sensor data
#define MSG_TYPE_STATUS     0x02    // Status update
#define MSG_TYPE_COMMAND    0x03    // Command
#define MSG_TYPE_ACK        0x04    // Acknowledgment
#define MSG_TYPE_ERROR      0x05    // Error message
```

### 3.4 WiFi State Machine

```c
typedef enum {
    WIFI_STATE_IDLE,            // WiFi disabled
    WIFI_STATE_SCANNING,        // Scanning for networks
    WIFI_STATE_CONNECTING,      // Connecting to AP
    WIFI_STATE_CONNECTED,       // Connected, no TCP
    WIFI_STATE_TCP_LISTENING,   // TCP server listening
    WIFI_STATE_TCP_CONNECTED,   // TCP client connected
    WIFI_STATE_STREAMING,       // Actively streaming data
    WIFI_STATE_ERROR            // Error state
} wifi_state_t;

typedef enum {
    WIFI_EVENT_START,           // Start WiFi
    WIFI_EVENT_STOP,            // Stop WiFi
    WIFI_EVENT_SCAN_DONE,       // Scan completed
    WIFI_EVENT_CONNECTED,       // WiFi connected
    WIFI_EVENT_DISCONNECTED,    // WiFi disconnected
    WIFI_EVENT_TCP_ACCEPT,      // TCP client accepted
    WIFI_EVENT_TCP_DISCONNECT,  // TCP client disconnected
    WIFI_EVENT_START_STREAM,    // Start streaming
    WIFI_EVENT_STOP_STREAM,     // Stop streaming
    WIFI_EVENT_ERROR            // Error occurred
} wifi_event_t;
```

---

## 4. Android App Enhancements

### 4.1 New Activities

#### WiFiConfigActivity
```java
public class WiFiConfigActivity extends AppCompatActivity {
    // WiFi configuration
    private EditText ssidInput;
    private EditText passwordInput;
    private Spinner modeSpinner;
    private Button connectButton;
    
    // Enable phone hotspot
    private void enableHotspot();
    
    // Configure ESP32 WiFi
    private void configureESP32WiFi(String ssid, String password);
    
    // Test connection
    private void testConnection();
}
```

#### ChartActivity
```java
public class ChartActivity extends AppCompatActivity {
    // Chart views
    private LineChart accelChart;
    private LineChart gyroChart;
    private LineChart magChart;
    private LineChart pressureChart;
    
    // Update charts with new data
    private void updateCharts(SensorSample sample);
    
    // Configure chart appearance
    private void setupCharts();
    
    // Export chart as image
    private void exportChart(LineChart chart);
}
```

### 4.2 WiFi Manager

```java
public class WiFiManager {
    private WifiManager wifiManager;
    private ConnectivityManager connectivityManager;
    
    // Enable WiFi hotspot
    public void enableHotspot(String ssid, String password);
    
    // Disable WiFi hotspot
    public void disableHotspot();
    
    // Get hotspot status
    public boolean isHotspotEnabled();
    
    // Get connected devices
    public List<String> getConnectedDevices();
    
    // WiFi Direct methods
    public void initWiFiDirect();
    public void createGroup();
    public void connectToGroup(WifiP2pDevice device);
}
```

### 4.3 TCP Client

```java
public class TCPClient {
    private Socket socket;
    private InputStream inputStream;
    private OutputStream outputStream;
    private Thread receiveThread;
    
    // Connect to ESP32
    public void connect(String ipAddress, int port);
    
    // Send command
    public void sendCommand(byte[] command);
    
    // Receive data
    public void startReceiving(DataCallback callback);
    
    // Disconnect
    public void disconnect();
    
    // Check connection status
    public boolean isConnected();
}
```

---

## 5. Data Visualization

### 5.1 Chart Library

**MPAndroidChart** - Open source charting library
- Version: 3.1.0+
- License: Apache 2.0
- Features: Real-time updates, zoom, pan, animations
- GitHub: https://github.com/PhilJay/MPAndroidChart

### 5.2 Chart Configuration

```java
// Accelerometer Chart (3 lines: X, Y, Z)
private void setupAccelChart() {
    LineChart chart = findViewById(R.id.accel_chart);
    
    // Chart appearance
    chart.setBackgroundColor(Color.WHITE);
    chart.setDrawGridBackground(false);
    chart.getDescription().setEnabled(false);
    chart.setTouchEnabled(true);
    chart.setDragEnabled(true);
    chart.setScaleEnabled(true);
    chart.setPinchZoom(true);
    
    // X-axis
    XAxis xAxis = chart.getXAxis();
    xAxis.setPosition(XAxis.XAxisPosition.BOTTOM);
    xAxis.setDrawGridLines(false);
    xAxis.setGranularity(0.1f);
    xAxis.setLabelCount(10);
    
    // Y-axis
    YAxis leftAxis = chart.getAxisLeft();
    leftAxis.setDrawGridLines(true);
    leftAxis.setAxisMinimum(-20f);
    leftAxis.setAxisMaximum(20f);
    
    YAxis rightAxis = chart.getAxisRight();
    rightAxis.setEnabled(false);
    
    // Legend
    Legend legend = chart.getLegend();
    legend.setForm(Legend.LegendForm.LINE);
    legend.setTextSize(12f);
    
    // Data sets
    LineDataSet dataSetX = new LineDataSet(null, "Accel X");
    dataSetX.setColor(Color.RED);
    dataSetX.setLineWidth(2f);
    dataSetX.setDrawCircles(false);
    dataSetX.setDrawValues(false);
    
    LineDataSet dataSetY = new LineDataSet(null, "Accel Y");
    dataSetY.setColor(Color.GREEN);
    dataSetY.setLineWidth(2f);
    dataSetY.setDrawCircles(false);
    dataSetY.setDrawValues(false);
    
    LineDataSet dataSetZ = new LineDataSet(null, "Accel Z");
    dataSetZ.setColor(Color.BLUE);
    dataSetZ.setLineWidth(2f);
    dataSetZ.setDrawCircles(false);
    dataSetZ.setDrawValues(false);
    
    LineData data = new LineData(dataSetX, dataSetY, dataSetZ);
    chart.setData(data);
}
```

### 5.3 Real-Time Update Strategy

```java
// Update chart with new sample
private void updateChart(LineChart chart, SensorSample sample) {
    LineData data = chart.getData();
    
    if (data != null) {
        // Get data sets
        ILineDataSet setX = data.getDataSetByIndex(0);
        ILineDataSet setY = data.getDataSetByIndex(1);
        ILineDataSet setZ = data.getDataSetByIndex(2);
        
        // Add new entries
        float time = sample.timestamp_us / 1000000.0f;  // Convert to seconds
        data.addEntry(new Entry(time, sample.accel_x_mg / 1000.0f), 0);
        data.addEntry(new Entry(time, sample.accel_y_mg / 1000.0f), 1);
        data.addEntry(new Entry(time, sample.accel_z_mg / 1000.0f), 2);
        
        // Notify data changed
        data.notifyDataChanged();
        chart.notifyDataSetChanged();
        
        // Limit visible range (e.g., last 10 seconds)
        chart.setVisibleXRangeMaximum(10.0f);
        chart.moveViewToX(time);
        
        // Remove old data (keep last 1000 points)
        if (setX.getEntryCount() > 1000) {
            setX.removeFirst();
            setY.removeFirst();
            setZ.removeFirst();
        }
    }
}
```

### 5.4 Chart Layout

```xml
<!-- activity_chart.xml -->
<ScrollView
    android:layout_width="match_parent"
    android:layout_height="match_parent">
    
    <LinearLayout
        android:layout_width="match_parent"
        android:layout_height="wrap_content"
        android:orientation="vertical"
        android:padding="16dp">
        
        <!-- Accelerometer Chart -->
        <TextView
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="Accelerometer (g)"
            android:textSize="18sp"
            android:textStyle="bold"/>
        
        <com.github.mikephil.charting.charts.LineChart
            android:id="@+id/accel_chart"
            android:layout_width="match_parent"
            android:layout_height="250dp"
            android:layout_marginTop="8dp"
            android:layout_marginBottom="16dp"/>
        
        <!-- Gyroscope Chart -->
        <TextView
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="Gyroscope (dps)"
            android:textSize="18sp"
            android:textStyle="bold"/>
        
        <com.github.mikephil.charting.charts.LineChart
            android:id="@+id/gyro_chart"
            android:layout_width="match_parent"
            android:layout_height="250dp"
            android:layout_marginTop="8dp"
            android:layout_marginBottom="16dp"/>
        
        <!-- Magnetometer Chart -->
        <TextView
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="Magnetometer (μT)"
            android:textSize="18sp"
            android:textStyle="bold"/>
        
        <com.github.mikephil.charting.charts.LineChart
            android:id="@+id/mag_chart"
            android:layout_width="match_parent"
            android:layout_height="250dp"
            android:layout_marginTop="8dp"
            android:layout_marginBottom="16dp"/>
        
        <!-- Pressure Chart -->
        <TextView
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="Pressure (hPa)"
            android:textSize="18sp"
            android:textStyle="bold"/>
        
        <com.github.mikephil.charting.charts.LineChart
            android:id="@+id/pressure_chart"
            android:layout_width="match_parent"
            android:layout_height="250dp"
            android:layout_marginTop="8dp"
            android:layout_marginBottom="16dp"/>
        
    </LinearLayout>
</ScrollView>
```

---

## 6. Implementation Details

### 6.1 ESP32 WiFi Service

**File**: `wifi_service.c/h`

```c
// Initialize WiFi service
esp_err_t wifi_service_init(void);

// Start WiFi in STA mode
esp_err_t wifi_service_start_sta(const char* ssid, const char* password);

// Start WiFi in AP mode
esp_err_t wifi_service_start_ap(const char* ssid, const char* password);

// Stop WiFi
esp_err_t wifi_service_stop(void);

// Get WiFi status
wifi_status_t wifi_service_get_status(void);

// TCP server functions
esp_err_t tcp_server_start(uint16_t port);
esp_err_t tcp_server_stop(void);
esp_err_t tcp_server_send_data(const uint8_t* data, size_t len);
bool tcp_server_is_client_connected(void);

// WiFi event callback
typedef void (*wifi_event_callback_t)(wifi_event_t event);
esp_err_t wifi_service_register_callback(wifi_event_callback_t callback);

// WiFi status structure
typedef struct {
    wifi_state_t state;
    char ssid[32];
    char ip_address[16];
    int8_t rssi;
    uint8_t channel;
    bool tcp_connected;
} wifi_status_t;
```

### 6.2 Communication Manager

**File**: `comm_manager.c/h`

```c
// Communication mode
typedef enum {
    COMM_MODE_NONE,         // No communication
    COMM_MODE_BLE,          // BLE communication
    COMM_MODE_WIFI          // WiFi communication
} comm_mode_t;

// Initialize communication manager
esp_err_t comm_manager_init(void);

// Set communication mode
esp_err_t comm_manager_set_mode(comm_mode_t mode);

// Get current mode
comm_mode_t comm_manager_get_mode(void);

// Send data (automatically uses active mode)
esp_err_t comm_manager_send_data(sensor_sample_t* sample);

// Send status
esp_err_t comm_manager_send_status(system_status_t* status);

// Check if connected
bool comm_manager_is_connected(void);
```

### 6.3 Android WiFi Integration

**File**: `WiFiService.java`

```java
public class WiFiService {
    private TCPClient tcpClient;
    private String esp32IpAddress;
    private int esp32Port = 8080;
    
    // Connect to ESP32
    public void connect(String ipAddress, int port, ConnectionCallback callback);
    
    // Disconnect
    public void disconnect();
    
    // Send command
    public void sendCommand(byte[] command);
    
    // Start receiving data
    public void startReceiving(DataCallback callback);
    
    // Get connection status
    public boolean isConnected();
    
    // Callbacks
    public interface ConnectionCallback {
        void onConnected();
        void onDisconnected();
        void onError(String error);
    }
    
    public interface DataCallback {
        void onDataReceived(SensorSample sample);
        void onStatusReceived(SystemStatus status);
    }
}
```

### 6.4 Data Analysis Features

```java
public class DataAnalyzer {
    // Calculate statistics
    public static Statistics calculateStats(List<Float> data);
    
    // Apply filters
    public static List<Float> applyLowPassFilter(List<Float> data, float alpha);
    public static List<Float> applyHighPassFilter(List<Float> data, float alpha);
    
    // FFT analysis (optional)
    public static FFTResult performFFT(List<Float> data);
    
    // Statistics structure
    public static class Statistics {
        public float min;
        public float max;
        public float mean;
        public float stdDev;
        public float rms;
    }
}
```

---

## 7. Testing & Validation

### 7.1 WiFi Communication Tests

| Test ID | Description | Pass Criteria |
|---------|-------------|---------------|
| WT-001 | WiFi STA connection | Connects in <5s |
| WT-002 | WiFi AP creation | AP visible |
| WT-003 | TCP server start | Server listening |
| WT-004 | TCP client connection | Connection successful |
| WT-005 | Data streaming | >100KB/s throughput |
| WT-006 | WiFi range | >20m range |
| WT-007 | Reconnection | Auto-reconnect works |
| WT-008 | WiFi Direct | P2P connection works |
| WT-009 | Stability | >99% uptime in 1hr |
| WT-010 | Fallback to BLE | BLE works if WiFi fails |

### 7.2 Visualization Tests

| Test ID | Description | Pass Criteria |
|---------|-------------|---------------|
| VT-001 | Chart initialization | All charts display |
| VT-002 | Real-time update | >10Hz update rate |
| VT-003 | Zoom functionality | Zoom works smoothly |
| VT-004 | Pan functionality | Pan works smoothly |
| VT-005 | Auto-scale | Y-axis scales correctly |
| VT-006 | Color coding | Colors distinct |
| VT-007 | Legend | Labels clear |
| VT-008 | Export | Image saved correctly |
| VT-009 | Performance | No lag with 1000 points |
| VT-010 | Memory usage | <100MB app memory |

### 7.3 Integration Tests

| Test ID | Description | Duration | Pass Criteria |
|---------|-------------|----------|---------------|
| IT-001 | End-to-end WiFi | 30 min | Complete workflow |
| IT-002 | Mode switching | 15 min | BLE↔WiFi works |
| IT-003 | Hotspot stability | 1 hour | No disconnections |
| IT-004 | Chart performance | 30 min | Smooth updates |
| IT-005 | Battery life | 35 min | System runs to completion |
| IT-006 | Data integrity | 30 min | No data corruption |

---

## 8. Project Structure

### 8.1 ESP32 Firmware Structure

```
Step1_PhaseC_DataLogger/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── README.md
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   └── app_config.h
├── components/
│   ├── wifi_service/
│   │   ├── CMakeLists.txt
│   │   ├── wifi_service.c
│   │   ├── wifi_service.h
│   │   └── tcp_server.c
│   ├── comm_manager/
│   │   ├── CMakeLists.txt
│   │   ├── comm_manager.c
│   │   └── comm_manager.h
│   └── [Phase A & B components...]
└── test/
    └── test_wifi.c
```

### 8.2 Android App Structure

```
DataLoggerApp_PhaseC/
├── app/
│   ├── build.gradle
│   ├── src/
│   │   ├── main/
│   │   │   ├── AndroidManifest.xml
│   │   │   ├── java/com/example/datalogger/
│   │   │   │   ├── [Phase B activities...]
│   │   │   │   ├── ChartActivity.java
│   │   │   │   ├── WiFiConfigActivity.java
│   │   │   │   ├── wifi/
│   │   │   │   │   ├── WiFiManager.java
│   │   │   │   │   ├── WiFiService.java
│   │   │   │   │   └── TCPClient.java
│   │   │   │   ├── chart/
│   │   │   │   │   ├── ChartManager.java
│   │   │   │   │   └── ChartUpdater.java
│   │   │   │   └── analysis/
│   │   │   │       ├── DataAnalyzer.java
│   │   │   │       └── FilterUtils.java
│   │   │   └── res/
│   │   │       ├── layout/
│   │   │       │   ├── activity_chart.xml
│   │   │       │   └── activity_wifi_config.xml
│   │   │       └── values/
│   │   │           └── strings.xml
│   │   └── test/
│   │       └── java/com/example/datalogger/
│   │           └── WiFiServiceTest.java
├── build.gradle
└── settings.gradle
```

---

## 9. Implementation Timeline

### Week 15: WiFi Stack Setup
**Days 1-2**: ESP-IDF WiFi Configuration
- [ ] Enable WiFi in sdkconfig
- [ ] Configure WiFi stack parameters
- [ ] Initialize WiFi controller
- [ ] Test basic WiFi functionality

**Days 3-4**: WiFi STA Mode
- [ ] Implement STA mode
- [ ] Implement connection logic
- [ ] Test connection to phone hotspot
- [ ] Handle disconnections

**Days 5-7**: TCP Server
- [ ] Implement TCP server
- [ ] Test client connections
- [ ] Implement data sending
- [ ] Performance testing

### Week 16: WiFi Data Transfer
**Days 1-2**: Protocol Implementation
- [ ] Implement packet format
- [ ] Implement send/receive logic
- [ ] Test data integrity
- [ ] Error handling

**Days 3-4**: Integration
- [ ] Integrate with Phase A/B code
- [ ] Add WiFi mode to state machine
- [ ] Test mode switching
- [ ] Verify all modes work

**Days 5-7**: Optimization
- [ ] Optimize throughput
- [ ] Reduce latency
- [ ] Test performance
- [ ] Power optimization

### Week 17: Android WiFi Integration
**Days 1-2**: WiFi Manager
- [ ] Implement hotspot control
- [ ] Test hotspot creation
- [ ] Handle permissions
- [ ] Test on multiple devices

**Days 3-4**: TCP Client
- [ ] Implement TCP client
- [ ] Test connection to ESP32
- [ ] Implement data reception
- [ ] Error handling

**Days 5-7**: Integration
- [ ] Integrate with existing app
- [ ] Add WiFi mode selection
- [ ] Test BLE/WiFi switching
- [ ] Bug fixes

### Week 18: Data Visualization
**Days 1-2**: Chart Library Setup
- [ ] Add MPAndroidChart dependency
- [ ] Create chart layouts
- [ ] Initialize charts
- [ ] Test basic display

**Days 3-4**: Real-Time Updates
- [ ] Implement chart update logic
- [ ] Test update performance
- [ ] Optimize rendering
- [ ] Handle large datasets

**Days 5-7**: Chart Features
- [ ] Implement zoom/pan
- [ ] Implement auto-scale
- [ ] Add export functionality
- [ ] Polish UI

### Week 19: Integration & Optimization
**Days 1-2**: End-to-End Testing
- [ ] Test complete WiFi workflow
- [ ] Test visualization
- [ ] Test on multiple devices
- [ ] Fix bugs

**Days 3-4**: Performance Optimization
- [ ] Optimize throughput
- [ ] Optimize chart rendering
- [ ] Reduce battery consumption
- [ ] Memory optimization

**Days 5-7**: Compatibility Testing
- [ ] Test on Android 5.0.1+
- [ ] Test different screen sizes
- [ ] Test different WiFi scenarios
- [ ] Document issues

### Week 20: Final Validation
**Days 1-2**: System Testing
- [ ] Complete system test
- [ ] All features test
- [ ] Performance validation
- [ ] Stress testing

**Days 3-4**: Documentation
- [ ] Complete user manual
- [ ] API documentation
- [ ] Troubleshooting guide
- [ ] Demo video

**Days 5-7**: Final Delivery
- [ ] Build final APK
- [ ] Package deliverables
- [ ] Final validation
- [ ] Project sign-off

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-05-03 | Bob (Advanced Mode) | Initial Phase C FSD |

---

**END OF PHASE C FSD**