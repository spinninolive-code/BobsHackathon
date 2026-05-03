# Functional Specification Document (FSD)
# Phase A: Local SD Card Storage
# IoT Multi-Sensor Data Logger

**Project Name**: IoT Multi-Sensor Data Logger - Phase A  
**Hardware Platform**: Unexpected Maker ProS3 (ESP32-S3)  
**Phase**: A - Local Storage  
**Document Version**: 1.0  
**Date**: 2026-05-03  
**Author**: Bob (Advanced Mode)  
**Status**: Ready for Implementation

---

## Table of Contents

1. [Phase A Overview](#1-phase-a-overview)
2. [Requirements](#2-requirements)
3. [Hardware Configuration](#3-hardware-configuration)
4. [Software Architecture](#4-software-architecture)
5. [Implementation Details](#5-implementation-details)
6. [Testing & Validation](#6-testing--validation)
7. [Project Structure](#7-project-structure)
8. [Implementation Timeline](#8-implementation-timeline)

---

## 1. Phase A Overview

### 1.1 Objectives
Phase A establishes the foundation of the IoT sensor data logger system with local SD card storage capabilities. This phase focuses on:
- High-speed sensor data acquisition (1kHz)
- Multi-tier buffering (PSRAM → NAND → SD)
- Reliable data storage in CSV format
- DIP switch mode control
- Battery monitoring
- Status indication

### 1.2 Success Criteria
- [ ] Achieve 1kHz sampling rate with <1% jitter
- [ ] Zero data loss during 8-hour continuous operation
- [ ] SD card write speed >500KB/s sustained
- [ ] Battery life >90 minutes continuous logging
- [ ] All sensors reading correctly
- [ ] CSV files properly formatted and readable

### 1.3 Deliverables
1. Complete ESP-IDF firmware project
2. Sensor driver libraries (ICM-42688-P, MMC5983MA, LPS22HB)
3. Buffer management system
4. SD card storage system
5. State machine implementation
6. Power management system
7. Test results and validation report
8. User documentation

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Acceptance Criteria |
|----|-------------|----------|---------------------|
| PA-FR-001 | Acquire data from both 10-DOF modules at 1kHz | CRITICAL | Verified with oscilloscope |
| PA-FR-002 | Buffer data in PSRAM (4MB ring buffer) | CRITICAL | No buffer overflows in 8hr test |
| PA-FR-003 | Cache data in NAND flash (8MB) | HIGH | Data integrity verified |
| PA-FR-004 | Write data to SD card in CSV format | CRITICAL | Files readable in Excel/Python |
| PA-FR-005 | Support DIP switch mode selection | HIGH | All 8 modes functional |
| PA-FR-006 | Monitor battery voltage continuously | HIGH | Accuracy ±50mV |
| PA-FR-007 | Generate timestamped filenames | MEDIUM | Format: YYYYMMDD_HHMMSS_MX.csv |
| PA-FR-008 | Prevent data loss during buffer overflow | CRITICAL | Automatic flush to SD |
| PA-FR-009 | Indicate status via RGB LED | MEDIUM | Clear visual feedback |
| PA-FR-010 | Handle SD card errors gracefully | HIGH | Retry logic, error logging |

### 2.2 Performance Requirements

| ID | Requirement | Target | Measurement Method |
|----|-------------|--------|-------------------|
| PA-PR-001 | Sample rate accuracy | 1000Hz ±10Hz | Oscilloscope measurement |
| PA-PR-002 | Sample rate jitter | <100μs RMS | Logic analyzer |
| PA-PR-003 | SD write speed | >500KB/s sustained | Benchmark test |
| PA-PR-004 | Buffer overflow prevention | 0 lost samples | 8-hour stress test |
| PA-PR-005 | Boot time | <5 seconds | Stopwatch |
| PA-PR-006 | Mode switch response | <500ms | Test harness |
| PA-PR-007 | Battery life (logging) | >90 minutes | Discharge test |
| PA-PR-008 | CPU utilization | <80% average | ESP-IDF profiler |

### 2.3 Non-Functional Requirements

| ID | Requirement | Description |
|----|-------------|-------------|
| PA-NFR-001 | Code Quality | Follow ESP-IDF coding standards |
| PA-NFR-002 | Documentation | Inline comments, README, API docs |
| PA-NFR-003 | Modularity | Separate drivers for each sensor |
| PA-NFR-004 | Testability | Unit tests for critical functions |
| PA-NFR-005 | Maintainability | Clear module boundaries |
| PA-NFR-006 | Reliability | Graceful error handling |

---

## 3. Hardware Configuration

### 3.1 Component List

#### Main Controller
- **Unexpected Maker ProS3** (ESP32-S3-WROOM-1-N16R8)
  - Dual-core 240MHz
  - 2MB SRAM, 8MB PSRAM, 16MB Flash
  - WiFi/BLE (not used in Phase A)

#### Sensor Modules (2x 10-DOF)
- **ICM-42688-P**: 6-axis IMU (accel + gyro)
- **MMC5983MA**: 3-axis magnetometer
- **LPS22HB**: Pressure + temperature sensor

#### Storage & Interface
- **SD Card Module**: SDIO 4-bit interface
- **DIP Switches**: 3-position mode selector
- **RGB LED**: Status indicator
- **LiPo Battery**: 220mAh, 3.7V

### 3.2 Pin Assignment

```
ESP32-S3 ProS3 Pin Configuration:

Sensor SPI Bus:
  GPIO35 -> SPI_MOSI
  GPIO36 -> SPI_MISO
  GPIO37 -> SPI_CLK (20MHz)
  GPIO1  -> SENSOR1_CS (ICM-42688-P #1)
  GPIO2  -> SENSOR2_CS (ICM-42688-P #2)
  GPIO3  -> MAG1_CS (MMC5983MA #1)
  GPIO4  -> MAG2_CS (MMC5983MA #2)
  GPIO5  -> PRESS1_CS (LPS22HB #1)
  GPIO6  -> PRESS2_CS (LPS22HB #2)

SD Card SDIO:
  GPIO12 -> SD_D0
  GPIO13 -> SD_D1
  GPIO14 -> SD_D2
  GPIO15 -> SD_D3
  GPIO16 -> SD_CLK (50MHz)
  GPIO17 -> SD_CMD

User Interface:
  GPIO7  -> DIP_SW1 (INPUT_PULLUP)
  GPIO8  -> DIP_SW2 (INPUT_PULLUP)
  GPIO9  -> DIP_SW3 (INPUT_PULLUP)
  GPIO11 -> STATUS_LED (WS2812 RGB)

Power:
  GPIO10 -> BATTERY_ADC (ADC1_CH0)
```

### 3.3 DIP Switch Mode Mapping

```
SW1 SW2 SW3 | Mode          | Description
----|----|----|--------------|---------------------------
 0   0   0  | OFF           | Deep sleep
 0   0   1  | IDLE          | Standby, ready to start
 0   1   0  | LOG_LOCAL     | Active logging to SD card
 0   1   1  | RESERVED      | (Future: BLE mode)
 1   0   0  | RESERVED      | (Future: WiFi mode)
 1   0   1  | CALIBRATE     | Sensor calibration
 1   1   0  | TEST          | Self-test mode
 1   1   1  | RESERVED      | Reserved for future use
```

---

## 4. Software Architecture

### 4.1 System Architecture

```
┌─────────────────────────────────────────────────┐
│         Application Layer                       │
│  - main.c (initialization, main loop)           │
│  - state_machine.c (mode control)               │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│         Data Management Layer                   │
│  - buffer_manager.c (PSRAM/NAND)                │
│  - sd_storage.c (CSV writer)                    │
│  - data_formatter.c (conversions)               │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│         Hardware Abstraction Layer              │
│  - icm42688_driver.c (accel/gyro)               │
│  - mmc5983_driver.c (magnetometer)              │
│  - lps22hb_driver.c (pressure)                  │
│  - power_manager.c (battery)                    │
│  - ui_manager.c (LED, switches)                 │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│         ESP-IDF / FreeRTOS                      │
│  - SPI driver, SDIO driver                      │
│  - GPIO, ADC, Timers                            │
│  - Task scheduler, Queues                       │
└─────────────────────────────────────────────────┘
```

### 4.2 FreeRTOS Task Structure

```c
// Task definitions
TaskHandle_t sensor_task_handle;
TaskHandle_t data_process_task_handle;
TaskHandle_t sd_write_task_handle;
TaskHandle_t ui_task_handle;
TaskHandle_t monitor_task_handle;

// Task priorities (0-24, higher = more priority)
#define TASK_PRIORITY_SENSOR    24  // Highest - time critical
#define TASK_PRIORITY_DATA      20  // High - data processing
#define TASK_PRIORITY_SD        15  // Medium-high - storage
#define TASK_PRIORITY_UI        8   // Medium-low - user interface
#define TASK_PRIORITY_MONITOR   5   // Low - battery/status

// Task stack sizes (bytes)
#define STACK_SIZE_SENSOR    4096
#define STACK_SIZE_DATA      8192
#define STACK_SIZE_SD        4096
#define STACK_SIZE_UI        2048
#define STACK_SIZE_MONITOR   2048
```

### 4.3 Data Flow

```
Sensor ISR (1kHz) → PSRAM Ring Buffer (4MB, ~58s)
                           ↓
                    NAND Flash Cache (8MB, ~116s)
                           ↓
                    SD Card (unlimited storage)

Trigger Points:
- PSRAM → NAND: When 50% full (2MB)
- NAND → SD: When 75% full (6MB)
- Emergency: When 90% full (immediate flush)
```

### 4.4 State Machine

```c
typedef enum {
    STATE_OFF,              // Deep sleep
    STATE_IDLE,             // Standby
    STATE_CALIBRATING,      // Sensor calibration
    STATE_LOGGING,          // Active logging
    STATE_TEST,             // Self-test
    STATE_ERROR,            // Error state
    STATE_LOW_BATTERY       // Low battery shutdown
} system_state_t;

typedef enum {
    EVENT_MODE_CHANGE,      // DIP switch changed
    EVENT_START_LOG,        // Start logging
    EVENT_STOP_LOG,         // Stop logging
    EVENT_BUFFER_FULL,      // Buffer overflow imminent
    EVENT_SD_FULL,          // SD card full
    EVENT_BATTERY_LOW,      // Battery below threshold
    EVENT_ERROR             // Error occurred
} system_event_t;
```

---

## 5. Implementation Details

### 5.1 Module Specifications

#### 5.1.1 Sensor Drivers

**File**: `icm42688_driver.c/h`
```c
// Initialize ICM-42688-P
esp_err_t icm42688_init(spi_device_handle_t spi, uint8_t cs_pin);

// Configure sensor
esp_err_t icm42688_config(icm42688_config_t* config);

// Read sensor data
esp_err_t icm42688_read_accel(int16_t* x, int16_t* y, int16_t* z);
esp_err_t icm42688_read_gyro(int16_t* x, int16_t* y, int16_t* z);
esp_err_t icm42688_read_all(icm42688_data_t* data);

// Configuration structure
typedef struct {
    uint8_t accel_range;    // 0=±2g, 1=±4g, 2=±8g, 3=±16g
    uint8_t gyro_range;     // 0=±250dps, 1=±500dps, 2=±1000dps, 3=±2000dps
    uint16_t accel_odr;     // Output data rate (Hz)
    uint16_t gyro_odr;      // Output data rate (Hz)
} icm42688_config_t;
```

**File**: `mmc5983_driver.c/h`
```c
// Initialize MMC5983MA
esp_err_t mmc5983_init(spi_device_handle_t spi, uint8_t cs_pin);

// Configure sensor
esp_err_t mmc5983_config(mmc5983_config_t* config);

// Read magnetometer data
esp_err_t mmc5983_read_mag(int32_t* x, int32_t* y, int32_t* z);

// Perform SET/RESET operation (for accuracy)
esp_err_t mmc5983_set_reset(void);
```

**File**: `lps22hb_driver.c/h`
```c
// Initialize LPS22HB
esp_err_t lps22hb_init(spi_device_handle_t spi, uint8_t cs_pin);

// Configure sensor
esp_err_t lps22hb_config(lps22hb_config_t* config);

// Read pressure and temperature
esp_err_t lps22hb_read_pressure(int32_t* pressure_raw);
esp_err_t lps22hb_read_temperature(int16_t* temp_raw);
esp_err_t lps22hb_read_all(lps22hb_data_t* data);
```

#### 5.1.2 Buffer Manager

**File**: `buffer_manager.c/h`
```c
// Initialize buffer system
esp_err_t buffer_init(void);

// Write sample to PSRAM buffer
esp_err_t buffer_write_sample(sensor_sample_t* sample);

// Read samples from buffer
esp_err_t buffer_read_samples(sensor_sample_t* samples, size_t count);

// Get buffer status
buffer_status_t buffer_get_status(void);

// Flush buffer to NAND/SD
esp_err_t buffer_flush(void);

typedef struct {
    uint32_t psram_used;        // Bytes used in PSRAM
    uint32_t psram_capacity;    // Total PSRAM capacity
    uint32_t nand_used;         // Bytes used in NAND
    uint32_t nand_capacity;     // Total NAND capacity
    uint32_t samples_buffered;  // Total samples in buffers
    bool overflow_warning;      // True if >80% full
} buffer_status_t;
```

#### 5.1.3 SD Storage

**File**: `sd_storage.c/h`
```c
// Initialize SD card
esp_err_t sd_init(void);

// Create new log file
esp_err_t sd_create_file(const char* filename);

// Write samples to file
esp_err_t sd_write_samples(sensor_sample_t* samples, size_t count);

// Close current file
esp_err_t sd_close_file(void);

// Get SD card info
sd_info_t sd_get_info(void);

typedef struct {
    uint64_t total_bytes;       // Total card capacity
    uint64_t free_bytes;        // Free space
    uint32_t files_created;     // Number of files created
    uint64_t bytes_written;     // Total bytes written
} sd_info_t;
```

#### 5.1.4 State Machine

**File**: `state_machine.c/h`
```c
// Initialize state machine
esp_err_t state_machine_init(void);

// Process event
esp_err_t state_machine_process_event(system_event_t event);

// Get current state
system_state_t state_machine_get_state(void);

// State transition callback
typedef void (*state_callback_t)(system_state_t old_state, system_state_t new_state);
esp_err_t state_machine_register_callback(state_callback_t callback);
```

#### 5.1.5 Power Manager

**File**: `power_manager.c/h`
```c
// Initialize power management
esp_err_t power_init(void);

// Read battery voltage
uint16_t power_read_battery_mv(void);

// Get battery percentage
uint8_t power_get_battery_percent(void);

// Check battery status
battery_status_t power_get_battery_status(void);

typedef enum {
    BATTERY_FULL,           // >4.1V
    BATTERY_GOOD,           // 3.7-4.1V
    BATTERY_LOW,            // 3.4-3.7V
    BATTERY_CRITICAL,       // 3.2-3.4V
    BATTERY_SHUTDOWN        // <3.2V
} battery_status_t;
```

### 5.2 Data Structures

```c
// Single sensor sample (one module)
typedef struct {
    uint32_t timestamp_us;      // Microsecond timestamp
    
    // ICM-42688-P
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;
    
    // MMC5983MA
    int32_t mag_x_raw;
    int32_t mag_y_raw;
    int32_t mag_z_raw;
    
    // LPS22HB
    int32_t pressure_raw;
    int16_t temperature_raw;
    
    uint8_t module_id;          // 0 or 1
    uint8_t status;             // Status flags
} __attribute__((packed)) sensor_sample_t;
// Size: 36 bytes per module, 72 bytes for both modules
```

### 5.3 CSV File Format

```csv
# IoT Sensor Data Logger - Phase A
# Hardware: Unexpected Maker ProS3 (ESP32-S3)
# Firmware Version: 1.0.0
# Module ID: 1
# Start Time: 2026-05-03T14:30:22Z
# Sample Rate: 1000 Hz
# Sensors: ICM-42688-P, MMC5983MA, LPS22HB
Timestamp_us,Accel_X_mg,Accel_Y_mg,Accel_Z_mg,Gyro_X_mdps,Gyro_Y_mdps,Gyro_Z_mdps,Mag_X_uT,Mag_Y_uT,Mag_Z_uT,Pressure_hPa,Temp_C
0,5,-3,1024,12,-8,5,45.2,-12.3,38.7,1013.25,22.5
1000,8,-5,1020,15,-10,8,45.3,-12.2,38.8,1013.26,22.5
```

---

## 6. Testing & Validation

### 6.1 Unit Tests

| Test ID | Module | Test Description | Pass Criteria |
|---------|--------|------------------|---------------|
| UT-001 | ICM42688 | Sensor initialization | WHO_AM_I = 0x47 |
| UT-002 | ICM42688 | Accel data read | Valid range values |
| UT-003 | MMC5983 | Magnetometer read | Non-zero values |
| UT-004 | LPS22HB | Pressure read | 900-1100 hPa |
| UT-005 | Buffer | PSRAM write/read | Data integrity |
| UT-006 | Buffer | Overflow detection | Warning triggered |
| UT-007 | SD | File creation | File exists |
| UT-008 | SD | CSV write | Valid format |
| UT-009 | Power | Battery read | Reasonable voltage |
| UT-010 | State | Mode transitions | Correct states |

### 6.2 Integration Tests

| Test ID | Description | Duration | Pass Criteria |
|---------|-------------|----------|---------------|
| IT-001 | Continuous logging | 1 hour | No errors, no data loss |
| IT-002 | Buffer stress test | 30 min | No overflows |
| IT-003 | SD write performance | 10 min | >500KB/s sustained |
| IT-004 | Mode switching | 15 min | All modes work |
| IT-005 | Battery discharge | 90 min | System runs to completion |
| IT-006 | Error recovery | 30 min | Graceful handling |

### 6.3 System Tests

| Test ID | Description | Expected Result |
|---------|-------------|-----------------|
| ST-001 | 8-hour stress test | Zero data loss, stable operation |
| ST-002 | Sample rate accuracy | 1000Hz ±1% |
| ST-003 | Timing jitter | <100μs RMS |
| ST-004 | Power consumption | <150mA average |
| ST-005 | SD card compatibility | Works with 3+ card brands |
| ST-006 | Temperature range | -10°C to +50°C operation |

---

## 7. Project Structure

```
Step1_PhaseA_DataLogger/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── README.md
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── app_config.h
│   └── Kconfig.projbuild
├── components/
│   ├── sensor_drivers/
│   │   ├── CMakeLists.txt
│   │   ├── icm42688_driver.c
│   │   ├── icm42688_driver.h
│   │   ├── mmc5983_driver.c
│   │   ├── mmc5983_driver.h
│   │   ├── lps22hb_driver.c
│   │   └── lps22hb_driver.h
│   ├── buffer_manager/
│   │   ├── CMakeLists.txt
│   │   ├── buffer_manager.c
│   │   └── buffer_manager.h
│   ├── sd_storage/
│   │   ├── CMakeLists.txt
│   │   ├── sd_storage.c
│   │   └── sd_storage.h
│   ├── state_machine/
│   │   ├── CMakeLists.txt
│   │   ├── state_machine.c
│   │   └── state_machine.h
│   ├── power_manager/
│   │   ├── CMakeLists.txt
│   │   ├── power_manager.c
│   │   └── power_manager.h
│   └── ui_manager/
│       ├── CMakeLists.txt
│       ├── ui_manager.c
│       └── ui_manager.h
├── test/
│   ├── test_sensors.c
│   ├── test_buffer.c
│   └── test_sd.c
└── docs/
    ├── HARDWARE_SETUP.md
    ├── API_REFERENCE.md
    └── TESTING_GUIDE.md
```

---

## 8. Implementation Timeline

### Week 1: Hardware Setup & Project Initialization
**Days 1-2**: Hardware Assembly
- [ ] Assemble ProS3 board with sensor modules
- [ ] Connect SD card module
- [ ] Wire DIP switches and LED
- [ ] Connect battery and test charging
- [ ] Verify all connections with multimeter

**Days 3-5**: ESP-IDF Project Setup
- [ ] Create ESP-IDF project structure
- [ ] Configure CMake build system
- [ ] Setup partition table (NVS, app, data cache)
- [ ] Configure sdkconfig (PSRAM, SDIO, SPI)
- [ ] Test basic GPIO and LED control

**Days 6-7**: Initial Testing
- [ ] Test SPI bus communication
- [ ] Test SDIO SD card detection
- [ ] Test DIP switch reading
- [ ] Test battery ADC reading
- [ ] Document hardware setup

### Week 2: Sensor Drivers
**Days 1-3**: ICM-42688-P Driver
- [ ] Implement SPI communication
- [ ] Implement WHO_AM_I check
- [ ] Implement configuration
- [ ] Implement accel/gyro read
- [ ] Test with single sensor

**Days 4-5**: MMC5983MA Driver
- [ ] Implement SPI communication
- [ ] Implement initialization
- [ ] Implement magnetometer read
- [ ] Implement SET/RESET operation
- [ ] Test with single sensor

**Days 6-7**: LPS22HB Driver
- [ ] Implement SPI communication
- [ ] Implement initialization
- [ ] Implement pressure/temp read
- [ ] Test all sensors together
- [ ] Verify no SPI conflicts

### Week 3: Data Acquisition
**Days 1-2**: Timer ISR Setup
- [ ] Configure high-resolution timer (1kHz)
- [ ] Implement ISR for sensor reading
- [ ] Test timing accuracy with oscilloscope
- [ ] Optimize ISR performance

**Days 3-4**: Multi-Sensor Reading
- [ ] Implement sequential sensor reads
- [ ] Test both sensor modules
- [ ] Verify data integrity
- [ ] Measure ISR execution time

**Days 5-7**: Data Formatting
- [ ] Implement raw-to-engineering conversions
- [ ] Create sensor_sample_t structure
- [ ] Test data formatting
- [ ] Performance optimization

### Week 4: Buffer Management
**Days 1-3**: PSRAM Ring Buffer
- [ ] Allocate PSRAM buffer (4MB)
- [ ] Implement ring buffer logic
- [ ] Implement thread-safe access
- [ ] Test write/read operations

**Days 4-5**: NAND Flash Cache
- [ ] Configure flash partition
- [ ] Implement flash write/read
- [ ] Test data persistence
- [ ] Verify wear leveling

**Days 6-7**: Buffer Coordination
- [ ] Implement PSRAM→NAND transfer
- [ ] Implement overflow detection
- [ ] Test buffer full scenarios
- [ ] Performance profiling

### Week 5: SD Card Storage
**Days 1-2**: SDIO Initialization
- [ ] Configure SDIO 4-bit mode
- [ ] Mount FAT32 filesystem
- [ ] Test card detection
- [ ] Test multiple card types

**Days 3-4**: CSV File Writer
- [ ] Implement file creation
- [ ] Implement CSV header generation
- [ ] Implement data formatting
- [ ] Test file writing

**Days 5-7**: Storage Integration
- [ ] Implement NAND→SD transfer
- [ ] Implement buffered writes
- [ ] Test write performance
- [ ] Error handling and recovery

### Week 6: State Machine & UI
**Days 1-3**: State Machine
- [ ] Implement state definitions
- [ ] Implement state transitions
- [ ] Implement event handling
- [ ] Test all states

**Days 4-5**: DIP Switch Control
- [ ] Implement switch polling
- [ ] Implement mode detection
- [ ] Implement debouncing
- [ ] Test mode switching

**Days 6-7**: LED Status
- [ ] Implement WS2812 driver
- [ ] Define status colors
- [ ] Implement status updates
- [ ] Test visual feedback

### Week 7: Power Management
**Days 1-2**: Battery Monitoring
- [ ] Implement ADC reading
- [ ] Implement voltage calculation
- [ ] Implement percentage calculation
- [ ] Test accuracy

**Days 3-4**: Power States
- [ ] Implement active state
- [ ] Implement idle state
- [ ] Implement deep sleep
- [ ] Test transitions

**Days 5-7**: Power Optimization
- [ ] Profile power consumption
- [ ] Optimize CPU frequency
- [ ] Optimize peripheral usage
- [ ] Test battery life

### Week 8: Testing & Validation
**Days 1-2**: Unit Testing
- [ ] Test all sensor drivers
- [ ] Test buffer management
- [ ] Test SD storage
- [ ] Fix bugs

**Days 3-4**: Integration Testing
- [ ] 1-hour continuous test
- [ ] Buffer stress test
- [ ] Mode switching test
- [ ] Error injection test

**Days 5-6**: System Testing
- [ ] 8-hour stress test
- [ ] Sample rate verification
- [ ] Power consumption test
- [ ] SD card compatibility test

**Day 7**: Documentation
- [ ] Complete API documentation
- [ ] Write user manual
- [ ] Create test report
- [ ] Phase A sign-off

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-05-03 | Bob (Advanced Mode) | Initial Phase A FSD |

---

**END OF PHASE A FSD**