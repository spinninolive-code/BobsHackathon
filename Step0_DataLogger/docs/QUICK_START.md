# Quick Start Guide

Get your Teensy 3.1 Data Logger up and running in 15 minutes!

## What You'll Need

### Hardware Checklist
- [ ] Teensy 3.1 board
- [ ] BMA250 accelerometer module
- [ ] SD card module (Adafruit #4682 or compatible)
- [ ] Tri-way switch
- [ ] 3× LEDs (Red, Green, Blue)
- [ ] 3× 220Ω resistors (for LEDs)
- [ ] 2× 10kΩ resistors (for voltage divider)
- [ ] Breadboard and jumper wires
- [ ] LiPo battery 3.7V 2000mAh
- [ ] SD card (8GB+, formatted FAT32)
- [ ] USB cable (for programming)

### Software Checklist
- [ ] Arduino IDE installed
- [ ] Teensyduino add-on installed
- [ ] USB drivers installed

## Step 1: Hardware Assembly (10 minutes)

### 1.1 Place Components on Breadboard

```
Breadboard Layout:
┌─────────────────────────────────────┐
│  [Teensy 3.1]  [BMA250]  [SD Card] │
│                                     │
│  [Switch]  [LEDs: G R B]           │
└─────────────────────────────────────┘
```

### 1.2 Connect Power Rails

```
Teensy 3.3V → Breadboard + rail (red)
Teensy GND  → Breadboard - rail (blue)
```

### 1.3 Connect BMA250 Accelerometer

```
BMA250 → Teensy
─────────────────
VCC    → 3.3V
GND    → GND
SDA    → Pin 18
SCL    → Pin 19
```

### 1.4 Connect SD Card Module

```
SD Card → Teensy
─────────────────
VCC     → 3.3V
GND     → GND
CS      → Pin 10
MOSI    → Pin 11
MISO    → Pin 12
SCK     → Pin 13
```

### 1.5 Connect Control Switch

```
Switch Position 1 → Pin 2 → GND (when closed)
Switch Position 2 → Pin 3 → GND (when closed)
Switch Position 3 → Pin 4 → GND (when closed)
```

### 1.6 Connect Status LEDs

```
Pin 5 → 220Ω → Green LED → GND  (Status)
Pin 6 → 220Ω → Red LED   → GND  (Error)
Pin 7 → 220Ω → Blue LED  → GND  (Logging)
```

### 1.7 Connect Battery Monitor

```
Battery+ ──┬── Teensy VIN
           │
           ├── 10kΩ ──┬── Pin A0
           │          │
           └── GND ───┴── 10kΩ ── GND
```

### 1.8 Visual Check

✅ All connections secure  
✅ No short circuits  
✅ Correct polarity on LEDs  
✅ 3.3V to all modules (not 5V!)

## Step 2: Software Setup (3 minutes)

### 2.1 Install Arduino IDE

1. Download from https://www.arduino.cc/en/software
2. Install for your operating system
3. Launch Arduino IDE

### 2.2 Install Teensyduino

1. Download from https://www.pjrc.com/teensy/td_download.html
2. Run installer
3. Select Arduino IDE location
4. Install all libraries (Wire, SPI, SD included)

### 2.3 Configure Arduino IDE

```
Tools → Board → Teensy 3.1
Tools → USB Type → Serial
Tools → CPU Speed → 48 MHz
Tools → Optimize → Faster
```

## Step 3: Upload Code (2 minutes)

### 3.1 Open Project

1. Open `Step0_DataLogger/src/DataLogger.ino`
2. Verify all files are loaded (check tabs at top)

### 3.2 Connect Teensy

1. Connect Teensy to computer via USB
2. Wait for drivers to install
3. Select port: `Tools → Port → (your Teensy)`

### 3.3 Upload

1. Click **Verify** button (✓) to compile
2. Wait for "Done compiling" message
3. Click **Upload** button (→)
4. Wait for "Done uploading" message

### 3.4 Monitor Serial Output

1. Open Serial Monitor: `Tools → Serial Monitor`
2. Set baud rate to **115200**
3. You should see initialization messages

```
=================================
Teensy 3.1 Data Logger
Firmware: 1.0.0
Date: 2026-05-03
=================================

Configuring pins...
Pins configured
Initializing data buffer...
DataBuffer initialized: 5461 samples capacity
Initializing accelerometer...
BMA250 chip ID verified: 0x03
...
Setup complete!
```

## Step 4: First Test (5 minutes)

### 4.1 Prepare SD Card

1. Format SD card as **FAT32**
2. Ensure card is empty or has space
3. Insert card into SD module

### 4.2 Power On

1. Set switch to position 2 (IDLE)
2. Connect battery or USB power
3. Green LED should blink slowly

### 4.3 Start Logging

1. Set switch to position 3 (LOGGING)
2. Blue LED should blink (heartbeat)
3. Green LED should be solid
4. Watch serial monitor for statistics

```
--- Statistics ---
Uptime: 10000 ms (10.0 sec)
Samples acquired: 10000
Samples written: 10000
Actual rate: 1000.0 Hz
Buffer usage: 9% (peak: 15%)
Acquisition errors: 0
Write errors: 0
Buffer overflows: 0
------------------
```

### 4.4 Stop Logging

1. Set switch to position 2 (IDLE)
2. Blue LED should turn off
3. Green LED should blink slowly
4. File is automatically closed

### 4.5 Check Data File

1. Remove SD card
2. Insert into computer
3. Open `LOG_0000.csv` in text editor or Excel
4. Verify data format:

```csv
# Teensy 3.1 Data Logger
# Firmware: 1.0.0
# Sample Rate: 1000 Hz
Timestamp_ms,Accel_X_mg,Accel_Y_mg,Accel_Z_mg
0,5,-3,1024
1,8,-5,1020
2,12,-8,1018
...
```

## Troubleshooting

### Problem: No serial output

**Check**:
- USB cable connected
- Correct port selected
- Baud rate set to 115200
- Serial Monitor open

### Problem: Accelerometer not found

**Check**:
- I2C connections (SDA=18, SCL=19)
- 3.3V power to BMA250
- BMA250 module is working
- Try different I2C address

### Problem: SD card not detected

**Check**:
- SPI connections (CS=10, MOSI=11, MISO=12, SCK=13)
- SD card formatted as FAT32
- SD card properly inserted
- Try different SD card

### Problem: LEDs not working

**Check**:
- LED polarity (long leg = +)
- 220Ω resistors in series
- Correct pins (5, 6, 7)
- LEDs not burned out

### Problem: Switch not responding

**Check**:
- Switch connections to pins 2, 3, 4
- Switch common to GND
- 50ms debounce working
- Try different switch

## Next Steps

### Experiment with Settings

Edit `src/config.h` to try:
- Different sample rates (100Hz, 500Hz, 2000Hz)
- Larger buffer sizes
- Different LED patterns
- Custom battery thresholds

### Analyze Your Data

Use Python, MATLAB, or Excel to:
- Plot acceleration vs time
- Calculate FFT for frequency analysis
- Detect motion events
- Compute statistics

### Example Python Analysis

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
data = pd.read_csv('LOG_0000.csv', comment='#')

# Plot X-axis
plt.plot(data['Timestamp_ms'], data['Accel_X_mg'])
plt.xlabel('Time (ms)')
plt.ylabel('Acceleration (mg)')
plt.title('X-axis Acceleration')
plt.show()
```

### Add Features

Ideas for enhancement:
- Real-time clock (RTC) for timestamps
- Wireless data transmission (Bluetooth/WiFi)
- Additional sensors (gyroscope, magnetometer)
- Data compression
- Web interface for configuration

## Safety Notes

⚠️ **Important Safety Information**:

1. **Battery Safety**:
   - Use proper LiPo battery with protection circuit
   - Never short circuit battery terminals
   - Charge with appropriate LiPo charger
   - Monitor battery temperature during use
   - Dispose of damaged batteries properly

2. **Electrical Safety**:
   - Always use 3.3V for Teensy and modules
   - Check polarity before connecting power
   - Avoid static discharge to components
   - Disconnect power before changing connections

3. **SD Card Safety**:
   - Always stop logging before removing card
   - Use proper eject procedure
   - Backup important data regularly
   - Format card if corruption occurs

## Support Resources

- **Documentation**: See `README.md` for full details
- **FSD**: See `docs/Step0_FSD.md` for specifications
- **Teensy Forum**: https://forum.pjrc.com/
- **Arduino Forum**: https://forum.arduino.cc/

## Success Checklist

After completing this guide, you should have:

- [x] Hardware assembled and connected
- [x] Software installed and configured
- [x] Code uploaded successfully
- [x] Serial monitor showing output
- [x] LEDs indicating status
- [x] Data logging to SD card
- [x] CSV file with valid data

**Congratulations! Your data logger is working!** 🎉

---

**Time to complete**: ~15 minutes  
**Difficulty**: Beginner to Intermediate  
**Last updated**: 2026-05-03