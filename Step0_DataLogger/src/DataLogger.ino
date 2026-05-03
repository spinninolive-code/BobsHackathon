/**
 * @file DataLogger.ino
 * @brief Main application for Teensy 3.1 Accelerometer Data Logger
 * @version 1.0.0
 * @date 2026-05-03
 * 
 * Hardware:
 * - Teensy 3.1 (MK20DX256)
 * - BMA250 Accelerometer (I2C)
 * - SD Card Module (SPI)
 * - Tri-way switch for control
 * - Status LEDs
 * 
 * Features:
 * - 1kHz data acquisition
 * - CSV file storage
 * - Battery monitoring
 * - Error handling and recovery
 */

#include "config.h"
#include "hardware/accelerometer.h"
#include "data/data_buffer.h"
#include <SD.h>
#include <SPI.h>

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

Accelerometer accel;
DataBuffer dataBuffer;
File dataFile;
IntervalTimer acquisitionTimer;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// System state
volatile SystemState currentState = STATE_OFF;
SystemState previousState = STATE_OFF;

// Timing
uint32_t startTime = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastSDWrite = 0;
uint32_t lastLEDUpdate = 0;
uint32_t lastSwitchCheck = 0;

// Statistics
uint32_t samplesAcquired = 0;
uint32_t samplesWritten = 0;
uint32_t writeErrors = 0;
uint32_t acquisitionErrors = 0;

// Battery
float batteryVoltage = 0.0;
uint8_t batteryPercent = 100;

// Flags
volatile bool bufferReadyToWrite = false;
volatile bool acquisitionActive = false;
bool sdCardPresent = false;
char currentFilename[SD_MAX_FILENAME_LEN];

// Switch state
uint8_t switchState = 0;
uint8_t lastSwitchState = 0;
uint32_t switchDebounceTime = 0;

// LED state
int ledStatusPattern = LED_PATTERN_OFF;
int ledErrorPattern = LED_PATTERN_OFF;
int ledLoggingPattern = LED_PATTERN_OFF;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void setup();
void loop();

// State machine
void handleStateTransition();
void enterState(SystemState newState);
void exitState(SystemState oldState);

// Data acquisition
void acquisitionISR();
void startAcquisition();
void stopAcquisition();

// SD card operations
bool initSDCard();
bool openLogFile();
bool writeBufferToSD();
void closeLogFile();
uint32_t getSDFreeSpace();

// Switch handling
uint8_t readSwitch();
void handleSwitchChange();

// Battery monitoring
void checkBattery();
float readBatteryVoltage();
uint8_t calculateBatteryPercent(float voltage);

// LED control
void updateLEDs();
void setLEDPattern(uint8_t led, int pattern);
void updateLED(uint8_t pin, int pattern);

// Error handling
void handleError(ErrorCode error);
void emergencyShutdown();

// Utility
void printStatus();
void printStatistics();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize serial for debugging
    #if DEBUG_ENABLED
    Serial.begin(DEBUG_SERIAL_BAUD);
    delay(1000); // Wait for serial to initialize
    DEBUG_PRINTLN("\n\n=================================");
    DEBUG_PRINTLN("Teensy 3.1 Data Logger");
    DEBUG_PRINTF("Firmware: %s\n", FIRMWARE_VERSION);
    DEBUG_PRINTF("Date: %s\n", FIRMWARE_DATE);
    DEBUG_PRINTLN("=================================\n");
    #endif
    
    // Configure pins
    DEBUG_PRINTLN("Configuring pins...");
    
    // Switch pins (INPUT_PULLUP)
    pinMode(PIN_SWITCH_OFF, INPUT_PULLUP);
    pinMode(PIN_SWITCH_IDLE, INPUT_PULLUP);
    pinMode(PIN_SWITCH_LOG, INPUT_PULLUP);
    
    // LED pins (OUTPUT)
    pinMode(PIN_LED_STATUS, OUTPUT);
    pinMode(PIN_LED_ERROR, OUTPUT);
    pinMode(PIN_LED_LOGGING, OUTPUT);
    
    // Turn off all LEDs initially
    digitalWrite(PIN_LED_STATUS, LOW);
    digitalWrite(PIN_LED_ERROR, LOW);
    digitalWrite(PIN_LED_LOGGING, LOW);
    
    // Battery monitor (ADC)
    pinMode(PIN_BATTERY_MON, INPUT);
    analogReadResolution(10); // 10-bit ADC
    
    DEBUG_PRINTLN("Pins configured");
    
    // Initialize data buffer
    DEBUG_PRINTLN("Initializing data buffer...");
    if (!dataBuffer.begin()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize data buffer");
        setLEDPattern(PIN_LED_ERROR, LED_PATTERN_ON);
        while(1); // Halt
    }
    
    // Initialize accelerometer
    DEBUG_PRINTLN("Initializing accelerometer...");
    if (!accel.begin()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize accelerometer");
        handleError(ERR_ACCEL_INIT);
        setLEDPattern(PIN_LED_ERROR, LED_PATTERN_FAST);
        // Continue anyway - will retry later
    } else {
        // Run self-test
        if (accel.selfTest()) {
            DEBUG_PRINTLN("Accelerometer self-test passed");
        } else {
            DEBUG_PRINTLN("WARNING: Accelerometer self-test failed");
        }
    }
    
    // Initialize SD card
    DEBUG_PRINTLN("Initializing SD card...");
    if (!initSDCard()) {
        DEBUG_PRINTLN("WARNING: SD card not detected");
        sdCardPresent = false;
        // Continue anyway - user can insert card later
    } else {
        sdCardPresent = true;
        DEBUG_PRINTF("SD card initialized, free space: %lu MB\n", getSDFreeSpace());
    }
    
    // Check battery
    checkBattery();
    DEBUG_PRINTF("Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
    
    // Read initial switch position
    switchState = readSwitch();
    lastSwitchState = switchState;
    DEBUG_PRINTF("Initial switch position: %d\n", switchState);
    
    // Enter initial state based on switch
    if (switchState == 1) {
        enterState(STATE_OFF);
    } else if (switchState == 2) {
        enterState(STATE_IDLE);
    } else if (switchState == 3) {
        // Don't start logging immediately - require transition from IDLE
        enterState(STATE_IDLE);
    }
    
    DEBUG_PRINTLN("\nSetup complete!\n");
    printStatus();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    uint32_t currentTime = millis();
    
    // Check switch state periodically
    if (currentTime - lastSwitchCheck >= SWITCH_SAMPLE_INTERVAL) {
        lastSwitchCheck = currentTime;
        handleSwitchChange();
    }
    
    // Handle state-specific tasks
    switch (currentState) {
        case STATE_OFF:
            // Minimal activity in OFF state
            delay(100);
            break;
            
        case STATE_IDLE:
            // Check battery periodically
            if (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL_MS) {
                lastBatteryCheck = currentTime;
                checkBattery();
            }
            
            // Update LEDs
            if (currentTime - lastLEDUpdate >= 100) {
                lastLEDUpdate = currentTime;
                updateLEDs();
            }
            break;
            
        case STATE_LOGGING:
            // Write buffer to SD if ready
            if (bufferReadyToWrite) {
                bufferReadyToWrite = false;
                
                if (!writeBufferToSD()) {
                    DEBUG_PRINTLN("ERROR: Failed to write to SD card");
                    writeErrors++;
                    handleError(ERR_SD_WRITE);
                }
            }
            
            // Check battery periodically
            if (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL_MS) {
                lastBatteryCheck = currentTime;
                checkBattery();
                
                // Check for low battery
                if (batteryVoltage < BATTERY_CRITICAL_V) {
                    DEBUG_PRINTLN("CRITICAL: Battery voltage too low!");
                    handleError(ERR_BATTERY_CRITICAL);
                    enterState(STATE_LOW_BATTERY);
                }
            }
            
            // Update LEDs
            if (currentTime - lastLEDUpdate >= 100) {
                lastLEDUpdate = currentTime;
                updateLEDs();
            }
            
            // Check buffer usage
            if (dataBuffer.getUsagePercent() > BUFFER_WARNING_PCT) {
                DEBUG_PRINTF("WARNING: Buffer %d%% full\n", dataBuffer.getUsagePercent());
                setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_VFAST);
            }
            
            // Periodic status update
            static uint32_t lastStatusPrint = 0;
            if (currentTime - lastStatusPrint >= 10000) { // Every 10 seconds
                lastStatusPrint = currentTime;
                printStatistics();
            }
            break;
            
        case STATE_ERROR:
            // Blink error LED
            updateLEDs();
            delay(100);
            
            // Try to recover after some time
            static uint32_t errorStartTime = 0;
            if (errorStartTime == 0) {
                errorStartTime = currentTime;
            }
            
            if (currentTime - errorStartTime > 5000) { // 5 seconds
                DEBUG_PRINTLN("Attempting recovery from error state...");
                errorStartTime = 0;
                enterState(STATE_IDLE);
            }
            break;
            
        case STATE_LOW_BATTERY:
            // Safe shutdown in progress
            updateLEDs();
            delay(100);
            break;
            
        case STATE_SHUTDOWN:
            // Final cleanup
            delay(100);
            break;
    }
}

// ============================================================================
// STATE MACHINE
// ============================================================================

void handleStateTransition() {
    SystemState newState = currentState;
    
    // Determine new state based on switch position
    if (switchState == 1) {
        newState = STATE_OFF;
    } else if (switchState == 2) {
        if (currentState == STATE_LOGGING) {
            newState = STATE_IDLE; // Stop logging
        } else if (currentState == STATE_OFF) {
            newState = STATE_IDLE; // Wake up
        }
    } else if (switchState == 3) {
        if (currentState == STATE_IDLE) {
            newState = STATE_LOGGING; // Start logging
        }
    }
    
    // Transition to new state if different
    if (newState != currentState) {
        DEBUG_PRINTF("State transition: %d -> %d\n", currentState, newState);
        exitState(currentState);
        enterState(newState);
    }
}

void enterState(SystemState newState) {
    previousState = currentState;
    currentState = newState;
    
    DEBUG_PRINTF("Entering state: %d\n", newState);
    
    switch (newState) {
        case STATE_OFF:
            // Turn off all LEDs
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_OFF);
            
            // Suspend accelerometer
            accel.suspend();
            
            DEBUG_PRINTLN("Entered OFF state");
            break;
            
        case STATE_IDLE:
            // Status LED slow blink
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_SLOW);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_OFF);
            
            // Wake up accelerometer if needed
            if (previousState == STATE_OFF) {
                accel.wakeup();
            }
            
            DEBUG_PRINTLN("Entered IDLE state - Ready to log");
            break;
            
        case STATE_LOGGING:
            // Check prerequisites
            if (!sdCardPresent) {
                DEBUG_PRINTLN("ERROR: Cannot start logging - no SD card");
                handleError(ERR_SD_INIT);
                enterState(STATE_ERROR);
                return;
            }
            
            if (batteryVoltage < BATTERY_LOW_V) {
                DEBUG_PRINTLN("WARNING: Low battery voltage");
                handleError(ERR_BATTERY_LOW);
            }
            
            // Open log file
            if (!openLogFile()) {
                DEBUG_PRINTLN("ERROR: Failed to open log file");
                handleError(ERR_FILE_CREATE);
                enterState(STATE_ERROR);
                return;
            }
            
            // Clear buffer
            dataBuffer.clear();
            
            // Reset statistics
            samplesAcquired = 0;
            samplesWritten = 0;
            writeErrors = 0;
            acquisitionErrors = 0;
            startTime = millis();
            
            // Start acquisition
            startAcquisition();
            
            // Set LED patterns
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_ON);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_MEDIUM);
            
            DEBUG_PRINTLN("Entered LOGGING state - Acquisition started");
            DEBUG_PRINTF("Log file: %s\n", currentFilename);
            break;
            
        case STATE_ERROR:
            // Fast blink error LED
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_FAST);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_OFF);
            
            DEBUG_PRINTLN("Entered ERROR state");
            break;
            
        case STATE_LOW_BATTERY:
            // Solid error LED
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_ON);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_OFF);
            
            DEBUG_PRINTLN("Entered LOW_BATTERY state - Shutting down");
            
            // Stop acquisition if active
            if (acquisitionActive) {
                stopAcquisition();
            }
            
            // Close file
            closeLogFile();
            
            // Enter shutdown state
            delay(1000);
            enterState(STATE_SHUTDOWN);
            break;
            
        case STATE_SHUTDOWN:
            // Turn off all LEDs
            setLEDPattern(PIN_LED_STATUS, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_ERROR, LED_PATTERN_OFF);
            setLEDPattern(PIN_LED_LOGGING, LED_PATTERN_OFF);
            
            DEBUG_PRINTLN("System shutdown complete");
            
            // Suspend accelerometer
            accel.suspend();
            
            // Enter low power mode
            enterState(STATE_OFF);
            break;
    }
}

void exitState(SystemState oldState) {
    DEBUG_PRINTF("Exiting state: %d\n", oldState);
    
    switch (oldState) {
        case STATE_LOGGING:
            // Stop acquisition
            stopAcquisition();
            
            // Write remaining buffer data
            if (dataBuffer.available() > 0) {
                DEBUG_PRINTLN("Writing remaining buffer data...");
                writeBufferToSD();
            }
            
            // Close log file
            closeLogFile();
            
            // Print final statistics
            printStatistics();
            break;
            
        default:
            break;
    }
}

// ============================================================================
// DATA ACQUISITION
// ============================================================================

void acquisitionISR() {
    // Read accelerometer
    int16_t x, y, z;
    
    if (accel.read(&x, &y, &z)) {
        // Create sample
        AccelSample sample;
        sample.timestamp_ms = millis() - startTime;
        sample.x = x;
        sample.y = y;
        sample.z = z;
        
        // Write to buffer
        if (dataBuffer.write(sample)) {
            samplesAcquired++;
            
            // Check if buffer ready to write
            if (dataBuffer.available() >= WRITE_THRESHOLD) {
                bufferReadyToWrite = true;
            }
        } else {
            // Buffer overflow
            acquisitionErrors++;
        }
    } else {
        // Read error
        acquisitionErrors++;
    }
}

void startAcquisition() {
    DEBUG_PRINTLN("Starting data acquisition...");
    
    // Configure timer for 1kHz (1000 microseconds)
    acquisitionTimer.begin(acquisitionISR, SAMPLE_INTERVAL_US);
    acquisitionTimer.priority(128); // High priority
    
    acquisitionActive = true;
    
    DEBUG_PRINTLN("Data acquisition started");
}

void stopAcquisition() {
    DEBUG_PRINTLN("Stopping data acquisition...");
    
    acquisitionTimer.end();
    acquisitionActive = false;
    
    DEBUG_PRINTLN("Data acquisition stopped");
}

// ============================================================================
// SD CARD OPERATIONS
// ============================================================================

bool initSDCard() {
    // Initialize SPI
    SPI.begin();
    
    // Initialize SD card
    if (!SD.begin(SD_CS_PIN)) {
        return false;
    }
    
    return true;
}

bool openLogFile() {
    // Generate filename with timestamp
    // Format: LOG_YYYYMMDD_HHMMSS.csv
    // For now, use simple counter since we don't have RTC
    static uint16_t fileCounter = 0;
    
    // Find next available filename
    do {
        snprintf(currentFilename, SD_MAX_FILENAME_LEN, 
                 "%s%04d%s", SD_FILENAME_PREFIX, fileCounter, SD_FILENAME_EXT);
        fileCounter++;
    } while (SD.exists(currentFilename) && fileCounter < 9999);
    
    // Open file for writing
    dataFile = SD.open(currentFilename, FILE_WRITE);
    
    if (!dataFile) {
        DEBUG_PRINTF("Failed to create file: %s\n", currentFilename);
        return false;
    }
    
    // Write CSV header
    dataFile.println("# Teensy 3.1 Data Logger");
    dataFile.printf("# Firmware: %s\n", FIRMWARE_VERSION);
    dataFile.printf("# Sample Rate: %d Hz\n", SAMPLE_RATE_HZ);
    dataFile.printf("# Range: ±2g\n");
    dataFile.printf("# Resolution: 10-bit\n");
    dataFile.printf("# Battery: %.2fV\n", batteryVoltage);
    dataFile.println("Timestamp_ms,Accel_X_mg,Accel_Y_mg,Accel_Z_mg");
    dataFile.flush();
    
    DEBUG_PRINTF("Log file created: %s\n", currentFilename);
    return true;
}

bool writeBufferToSD() {
    if (!dataFile) {
        return false;
    }
    
    uint32_t writeStart = micros();
    
    // Read samples from buffer
    AccelSample samples[WRITE_THRESHOLD];
    uint16_t count = dataBuffer.readMultiple(samples, WRITE_THRESHOLD);
    
    if (count == 0) {
        return true; // Nothing to write
    }
    
    // Write samples to file
    for (uint16_t i = 0; i < count; i++) {
        // Convert to milligravity
        int16_t x_mg = accel.rawToMilliG(samples[i].x);
        int16_t y_mg = accel.rawToMilliG(samples[i].y);
        int16_t z_mg = accel.rawToMilliG(samples[i].z);
        
        // Write CSV line
        dataFile.print(samples[i].timestamp_ms);
        dataFile.print(',');
        dataFile.print(x_mg);
        dataFile.print(',');
        dataFile.print(y_mg);
        dataFile.print(',');
        dataFile.println(z_mg);
    }
    
    // Flush to ensure data is written
    dataFile.flush();
    
    samplesWritten += count;
    
    uint32_t writeTime = micros() - writeStart;
    
    #if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
    DEBUG_PRINTF("Wrote %d samples in %lu us\n", count, writeTime);
    #endif
    
    return true;
}

void closeLogFile() {
    if (dataFile) {
        DEBUG_PRINTLN("Closing log file...");
        
        // Write footer
        dataFile.println("# End of log");
        dataFile.printf("# Total samples: %lu\n", samplesWritten);
        dataFile.printf("# Duration: %lu ms\n", millis() - startTime);
        
        dataFile.close();
        DEBUG_PRINTLN("Log file closed");
    }
}

uint32_t getSDFreeSpace() {
    // This is a simplified version - actual implementation would
    // query the SD card for free space
    return 1000; // Return 1000 MB as placeholder
}

// ============================================================================
// SWITCH HANDLING
// ============================================================================

uint8_t readSwitch() {
    // Read switch pins (active low with pullup)
    bool off = !digitalRead(PIN_SWITCH_OFF);
    bool idle = !digitalRead(PIN_SWITCH_IDLE);
    bool log = !digitalRead(PIN_SWITCH_LOG);
    
    // Determine position (1=OFF, 2=IDLE, 3=LOG)
    if (off) return 1;
    if (idle) return 2;
    if (log) return 3;
    
    return 0; // Invalid/no position
}

void handleSwitchChange() {
    uint8_t newState = readSwitch();
    
    // Debounce
    if (newState != lastSwitchState) {
        if (millis() - switchDebounceTime > SWITCH_DEBOUNCE_MS) {
            switchDebounceTime = millis();
            
            if (newState != switchState) {
                DEBUG_PRINTF("Switch changed: %d -> %d\n", switchState, newState);
                switchState = newState;
                handleStateTransition();
            }
        }
        lastSwitchState = newState;
    }
}

// ============================================================================
// BATTERY MONITORING
// ============================================================================

void checkBattery() {
    batteryVoltage = readBatteryVoltage();
    batteryPercent = calculateBatteryPercent(batteryVoltage);
    
    #if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
    DEBUG_PRINTF("Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
    #endif
}

float readBatteryVoltage() {
    // Read ADC multiple times and average
    uint32_t sum = 0;
    for (int i = 0; i < BATTERY_ADC_SAMPLES; i++) {
        sum += analogRead(PIN_BATTERY_MON);
        delayMicroseconds(100);
    }
    
    uint16_t adcValue = sum / BATTERY_ADC_SAMPLES;
    
    // Convert to voltage (voltage divider × 2)
    float voltage = (adcValue / 1023.0) * 3.3 * BATTERY_VOLTAGE_DIVIDER;
    
    return voltage;
}

uint8_t calculateBatteryPercent(float voltage) {
    // LiPo discharge curve approximation
    if (voltage >= 4.2) return 100;
    if (voltage >= 4.0) return 90;
    if (voltage >= 3.9) return 80;
    if (voltage >= 3.8) return 60;
    if (voltage >= 3.7) return 40;
    if (voltage >= 3.6) return 20;
    if (voltage >= 3.5) return 10;
    if (voltage >= 3.3) return 5;
    return 0;
}

// ============================================================================
// LED CONTROL
// ============================================================================

void updateLEDs() {
    updateLED(PIN_LED_STATUS, ledStatusPattern);
    updateLED(PIN_LED_ERROR, ledErrorPattern);
    updateLED(PIN_LED_LOGGING, ledLoggingPattern);
}

void setLEDPattern(uint8_t led, int pattern) {
    if (led == PIN_LED_STATUS) {
        ledStatusPattern = pattern;
    } else if (led == PIN_LED_ERROR) {
        ledErrorPattern = pattern;
    } else if (led == PIN_LED_LOGGING) {
        ledLoggingPattern = pattern;
    }
}

void updateLED(uint8_t pin, int pattern) {
    if (pattern == LED_PATTERN_OFF) {
        digitalWrite(pin, LOW);
    } else if (pattern == LED_PATTERN_ON) {
        digitalWrite(pin, HIGH);
    } else {
        // Blink pattern
        uint32_t period = 1000 / pattern; // Period in ms
        uint32_t phase = millis() % period;
        digitalWrite(pin, (phase < period / 2) ? HIGH : LOW);
    }
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

void handleError(ErrorCode error) {
    DEBUG_PRINTF("Error: 0x%02X\n", error);
    
    // Log error (simplified - would write to error log file)
    
    // Take action based on error severity
    switch (error) {
        case ERR_BATTERY_CRITICAL:
            emergencyShutdown();
            break;
            
        case ERR_SD_FULL:
            stopAcquisition();
            closeLogFile();
            enterState(STATE_ERROR);
            break;
            
        case ERR_BUFFER_OVERFLOW:
            // This is critical - we're losing data
            DEBUG_PRINTLN("CRITICAL: Buffer overflow!");
            break;
            
        default:
            // Non-critical errors - continue operation
            break;
    }
}

void emergencyShutdown() {
    DEBUG_PRINTLN("EMERGENCY SHUTDOWN!");
    
    // Stop acquisition immediately
    if (acquisitionActive) {
        acquisitionTimer.end();
        acquisitionActive = false;
    }
    
    // Try to save data
    if (dataFile) {
        writeBufferToSD();
        dataFile.println("# Emergency shutdown");
        dataFile.close();
    }
    
    // Enter shutdown state
    enterState(STATE_SHUTDOWN);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void printStatus() {
    DEBUG_PRINTLN("\n--- System Status ---");
    DEBUG_PRINTF("State: %d\n", currentState);
    DEBUG_PRINTF("Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
    DEBUG_PRINTF("SD Card: %s\n", sdCardPresent ? "Present" : "Not detected");
    DEBUG_PRINTF("Accelerometer: %s\n", accel.isConnected() ? "Connected" : "Not responding");
    DEBUG_PRINTLN("--------------------\n");
}

void printStatistics() {
    uint32_t uptime = millis() - startTime;
    float duration_sec = uptime / 1000.0;
    float actual_rate = samplesAcquired / duration_sec;
    
    DEBUG_PRINTLN("\n--- Statistics ---");
    DEBUG_PRINTF("Uptime: %lu ms (%.1f sec)\n", uptime, duration_sec);
    DEBUG_PRINTF("Samples acquired: %lu\n", samplesAcquired);
    DEBUG_PRINTF("Samples written: %lu\n", samplesWritten);
    DEBUG_PRINTF("Actual rate: %.1f Hz\n", actual_rate);
    DEBUG_PRINTF("Buffer usage: %d%% (peak: %d%%)\n", 
                 dataBuffer.getUsagePercent(),
                 (dataBuffer.getPeakUsage() * 100) / BUFFER_SIZE);
    DEBUG_PRINTF("Acquisition errors: %lu\n", acquisitionErrors);
    DEBUG_PRINTF("Write errors: %lu\n", writeErrors);
    DEBUG_PRINTF("Buffer overflows: %lu\n", dataBuffer.getOverflowCount());
    DEBUG_PRINTLN("------------------\n");
}

// Made with Bob
