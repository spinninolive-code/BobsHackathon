# Phase A: IoT Multi-Sensor Data Logger
## Local SD Card Storage

This is the Phase A implementation of the IoT Multi-Sensor Data Logger project for the ESP32-S3 platform.

## Features

- **High-Speed Data Acquisition**: 1kHz sampling rate from dual 10-DOF sensor modules
- **Multi-Tier Buffering**: PSRAM (4MB) → NAND Flash (8MB) → SD Card
- **Sensor Support**:
  - ICM-42688-P: 6-axis IMU (accelerometer + gyroscope)
  - MMC5983MA: 3-axis magnetometer
  - LPS22HB: Pressure + temperature sensor
- **DIP Switch Control**: 8 operating modes
- **Battery Monitoring**: Real-time voltage and percentage
- **CSV Data Format**: Standard format for easy analysis

## Hardware Requirements

- Unexpected Maker ProS3 (ESP32-S3)
- 2× 10-DOF sensor modules (ICM-42688-P, MMC5983MA, LPS22HB)
- SD card module (SDIO interface)
- 3-position DIP switches
- RGB LED (WS2812)
- 220mAh LiPo battery

## Pin Configuration

See [`Step1_PhaseA_FSD.md`](../Step1_PhaseA_FSD.md) for complete pin assignments.

## Building

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Configure project
cd Step1_PhaseA_DataLogger
idf.py set-target esp32s3
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Structure

```
Step1_PhaseA_DataLogger/
├── main/                   # Main application
├── components/             # Reusable components
│   ├── sensor_drivers/     # ICM, MMC, LPS drivers
│   ├── buffer_manager/     # PSRAM/NAND buffering
│   ├── sd_storage/         # SD card and CSV writer
│   ├── state_machine/      # System state management
│   ├── power_manager/      # Battery monitoring
│   └── ui_manager/         # LED and DIP switches
├── test/                   # Unit tests
└── docs/                   # Documentation
```

## Usage

### DIP Switch Modes

| SW1 | SW2 | SW3 | Mode |
|-----|-----|-----|------|
| 0 | 0 | 0 | OFF (deep sleep) |
| 0 | 0 | 1 | IDLE (standby) |
| 0 | 1 | 0 | LOG_LOCAL (active logging) |
| 0 | 1 | 1 | Reserved (Phase B: BLE) |
| 1 | 0 | 0 | Reserved (Phase C: WiFi) |
| 1 | 0 | 1 | CALIBRATE |
| 1 | 1 | 0 | TEST (self-test) |
| 1 | 1 | 1 | Reserved |

### CSV Output Format

Files are created with timestamp: `YYYYMMDD_HHMMSS_M1.csv`

```csv
Timestamp_us,Accel_X_mg,Accel_Y_mg,Accel_Z_mg,Gyro_X_mdps,Gyro_Y_mdps,Gyro_Z_mdps,Mag_X_uT,Mag_Y_uT,Mag_Z_uT,Pressure_hPa,Temp_C
0,5,-3,1024,12,-8,5,45.2,-12.3,38.7,1013.25,22.5
```

## Testing

```bash
# Run unit tests
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# Monitor output
idf.py -p /dev/ttyUSB0 monitor
```

## Performance Targets

- Sample rate: 1000Hz ±1%
- Jitter: <100μs RMS
- SD write speed: >500KB/s sustained
- Battery life: >90 minutes continuous logging
- Zero data loss in 8-hour operation

## Troubleshooting

### SD Card Not Detected
- Check SDIO connections (GPIO 12-17)
- Verify SD card is formatted as FAT32
- Try different SD card (Class 10 recommended)

### Sensor Read Errors
- Verify SPI connections (GPIO 35-37)
- Check CS pin assignments (GPIO 1-6)
- Verify 3.3V power supply

### Buffer Overflow
- Check SD card write speed
- Verify PSRAM is enabled in sdkconfig
- Monitor buffer fill levels in logs

## License

See project root for license information.

## Next Steps

After Phase A is complete and validated:
- **Phase B**: Add BLE communication
- **Phase C**: Add WiFi and real-time visualization

## References

- [Phase A FSD](../Step1_PhaseA_FSD.md)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [Unexpected Maker ProS3](https://esp32s3.com/pros3d.html)