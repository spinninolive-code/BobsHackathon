# Phase C: WiFi Communication & Visualization Data Logger

## Overview

Phase C extends Phase B by adding WiFi communication capabilities and real-time data visualization. The ESP32-S3 now acts as a WiFi Access Point (or connects to an existing network), allowing the Android app to communicate over WiFi instead of BLE. This dramatically increases data throughput and enables advanced visualization features.

## Key Features

### ESP32-S3 Firmware
- WiFi SoftAP mode (ESP32 as access point)
- WiFi Station mode (connect to existing network)
- WiFi Direct support
- HTTP/WebSocket server for data streaming
- RESTful API for control and status
- Real-time data streaming at full 1kHz rate
- Web-based configuration interface
- Backward compatible with Phase A & B

### Android Application
- WiFi connection management
- Real-time data visualization with charts
- Multiple chart types (line, scatter, FFT)
- Configurable display parameters
- Data recording and playback
- Export to CSV/JSON
- Enhanced UI with Material Design

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-S3 Firmware                        │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │  Phase A/B │  │  WiFi Stack  │  │  HTTP/WS     │        │
│  │ Components │→ │   SoftAP     │→ │   Server     │        │
│  └────────────┘  └──────────────┘  └──────────────┘        │
│                          ↓                                   │
└──────────────────────────┼───────────────────────────────────┘
                           │ WiFi (802.11n, 2.4GHz)
                           ↓
┌─────────────────────────────────────────────────────────────┐
│                    Android Application                       │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │  WiFi      │  │  WebSocket   │  │  Real-time   │        │
│  │  Manager   │→ │  Client      │→ │  Charts      │        │
│  └────────────┘  └──────────────┘  └──────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

## Project Structure

```
Step1_PhaseC_WiFi/
├── CMakeLists.txt              # Project config (includes Phase A & B)
├── sdkconfig.defaults          # WiFi-enabled configuration
├── partitions.csv              # Flash partition table
├── README.md                   # This file
├── BUILD_AND_TEST.md          # Build and test guide
├── main/
│   ├── CMakeLists.txt         # Main component
│   └── main.c                 # WiFi-integrated application
├── components/
│   ├── wifi_service/          # WiFi management component
│   │   ├── CMakeLists.txt
│   │   ├── wifi_service.h
│   │   └── wifi_service.c
│   └── web_server/            # HTTP/WebSocket server
│       ├── CMakeLists.txt
│       ├── web_server.h
│       ├── web_server.c
│       └── www/               # Web interface files
│           ├── index.html
│           ├── style.css
│           └── app.js
├── android_app/               # Enhanced Android app
│   ├── app/
│   │   ├── build.gradle
│   │   └── src/
│   │       └── main/
│   │           ├── AndroidManifest.xml
│   │           ├── java/
│   │           │   └── com/datalogger/
│   │           │       ├── MainActivity.kt
│   │           │       ├── WiFiManager.kt
│   │           │       ├── WebSocketClient.kt
│   │           │       ├── ChartActivity.kt
│   │           │       └── models/
│   │           └── res/
│   │               ├── layout/
│   │               ├── values/
│   │               └── drawable/
│   ├── build.gradle
│   └── settings.gradle
└── docs/
    ├── WIFI_PROTOCOL.md       # WiFi communication protocol
    ├── API_REFERENCE.md       # REST API documentation
    └── VISUALIZATION.md       # Chart configuration guide
```

## WiFi Configuration

### SoftAP Mode (Default)
- SSID: `DataLogger_XXXX` (XXXX = last 4 digits of MAC)
- Password: `datalogger123` (configurable)
- IP Address: `192.168.4.1`
- DHCP Server: Enabled
- Channel: Auto (1-11)
- Max Connections: 4

### Station Mode
- Connect to existing WiFi network
- DHCP client or static IP
- mDNS hostname: `datalogger.local`

### WiFi Direct
- P2P group owner mode
- Automatic IP assignment
- WPS support

## Communication Protocol

### HTTP REST API

#### Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Get system status |
| POST | `/api/start` | Start acquisition |
| POST | `/api/stop` | Stop acquisition |
| GET | `/api/files` | List SD card files |
| GET | `/api/files/{name}` | Download file |
| DELETE | `/api/files/{name}` | Delete file |
| POST | `/api/clear` | Clear buffers |
| GET | `/api/config` | Get configuration |
| POST | `/api/config` | Update configuration |
| GET | `/api/wifi` | Get WiFi status |
| POST | `/api/wifi` | Configure WiFi |

#### Example Requests

```bash
# Get status
curl http://192.168.4.1/api/status

# Start acquisition
curl -X POST http://192.168.4.1/api/start

# Get file list
curl http://192.168.4.1/api/files
```

### WebSocket Protocol

#### Connection
```
ws://192.168.4.1/ws
```

#### Message Types

**From ESP32 to Android:**
```json
{
  "type": "data",
  "timestamp": 1234567890,
  "samples": [
    {
      "module": 1,
      "accel": [0.1, -0.2, 0.98],
      "gyro": [1.2, -2.3, 3.4],
      "mag": [45.6, -56.7, 67.8],
      "pressure": 1013.25,
      "temperature": 23.5
    }
  ]
}
```

**From Android to ESP32:**
```json
{
  "type": "command",
  "command": "start",
  "params": {}
}
```

### Data Streaming

#### Binary Format (High Performance)
- Header: 4 bytes (magic + length)
- Payload: N × 50 bytes (sensor samples)
- CRC: 2 bytes

#### JSON Format (Human Readable)
- Batch of samples in JSON array
- Compressed with gzip
- Suitable for web interface

## Performance Specifications

| Metric | Phase B (BLE) | Phase C (WiFi) | Improvement |
|--------|---------------|----------------|-------------|
| Throughput | ~20 KB/s | ~1 MB/s | 50x |
| Latency | ~50ms | ~10ms | 5x |
| Range | ~10m | ~50m | 5x |
| Streaming Rate | ~300 Hz | 1000 Hz | 3.3x |
| Connections | 1 | 4 | 4x |

## Android App Features

### Main Screen
- WiFi network scanner
- Connect to DataLogger AP
- Connection status indicator
- Quick start/stop buttons
- Battery and storage indicators

### Real-Time Visualization
- **Line Charts**: Time-series data
- **Scatter Plots**: 3D acceleration/gyroscope
- **FFT Analysis**: Frequency domain
- **Waterfall Display**: Spectrogram
- **Configurable**: Axis ranges, colors, update rate

### Chart Configuration
- Select sensors to display
- Choose chart type
- Set time window (1s to 60s)
- Adjust update rate (10Hz to 60Hz)
- Enable/disable grid, legend
- Export chart as image

### Data Management
- Record sessions with metadata
- Playback recorded data
- Export to CSV/JSON/MAT
- Share via email/cloud
- Delete old recordings

### Settings
- WiFi connection preferences
- Chart default settings
- Data storage location
- Auto-connect options
- Theme selection (light/dark)

## Development Requirements

### ESP32-S3 Firmware
- ESP-IDF v5.1+
- WiFi enabled in sdkconfig
- HTTP server component
- WebSocket support
- All Phase A & B components

### Android App
- Android Studio Flamingo or later
- Minimum SDK: API 21 (Android 5.0.1)
- Target SDK: API 33 (Android 13)
- Kotlin 1.8+
- Jetpack Compose for UI
- MPAndroidChart for visualization

### Libraries
- **ESP32**: esp_http_server, esp_websocket_client
- **Android**: OkHttp, Scarlet (WebSocket), MPAndroidChart

## Building

### ESP32-S3 Firmware
```bash
cd Step1_PhaseC_WiFi
idf.py set-target esp32s3
idf.py menuconfig  # Configure WiFi settings
idf.py build
idf.py flash monitor
```

### Android App
```bash
cd Step1_PhaseC_WiFi/android_app
./gradlew assembleDebug
# APK in app/build/outputs/apk/debug/
```

## Installation

### ESP32-S3
Flash firmware via USB as in previous phases.

### Android App
1. Copy APK to phone
2. Install (allow unknown sources)
3. Grant WiFi and Storage permissions
4. Launch app

## Testing

### WiFi Connection Test
1. Flash ESP32-S3 firmware
2. Power on device
3. Scan for WiFi networks on phone
4. Connect to `DataLogger_XXXX`
5. Open browser to `http://192.168.4.1`
6. Verify web interface loads

### Data Streaming Test
1. Connect to DataLogger WiFi
2. Open Android app
3. Press "Start" button
4. Verify real-time charts update
5. Check data rate indicator
6. Press "Stop" button

### Visualization Test
1. Start data acquisition
2. Open chart configuration
3. Select different chart types
4. Adjust time windows
5. Enable FFT analysis
6. Verify smooth updates

## Web Interface

The ESP32 hosts a web interface accessible at `http://192.168.4.1`:

### Features
- System status dashboard
- Real-time data charts (using Chart.js)
- Start/stop controls
- File management
- WiFi configuration
- Firmware update (OTA)

### Technologies
- HTML5 + CSS3
- JavaScript (ES6+)
- Chart.js for visualization
- WebSocket for real-time data
- Responsive design (mobile-friendly)

## Compatibility

### Tested Devices
- Samsung Galaxy S4 GT-I9500 (Android 5.0.1)
- Samsung Galaxy S7 (Android 8.0)
- Google Pixel 3 (Android 11)
- OnePlus 7 Pro (Android 12)

### Known Issues
- Some Android 5.x devices have WiFi Direct issues
- Chart performance may be limited on older devices
- WebSocket reconnection may be slow on some networks

## Security

### Authentication
- WPA2-PSK for WiFi
- Optional HTTP Basic Auth
- API key support
- HTTPS (optional, with self-signed cert)

### Best Practices
- Change default WiFi password
- Enable API authentication
- Use HTTPS for sensitive data
- Regularly update firmware

## Next Steps

After Phase C completion:
1. Optimize visualization performance
2. Add machine learning features
3. Implement cloud sync
4. Add multi-device support
5. Create desktop application

## References

- Phase A Implementation: `../Step1_PhaseA_DataLogger/`
- Phase B Implementation: `../Step1_PhaseB_BLE/`
- Phase C FSD: `../Step1_PhaseC_FSD.md`
- WiFi Protocol: `docs/WIFI_PROTOCOL.md`
- API Reference: `docs/API_REFERENCE.md`
- Visualization Guide: `docs/VISUALIZATION.md`