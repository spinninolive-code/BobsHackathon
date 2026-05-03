package com.esp32.sensorlogger

import android.app.Application
import timber.log.Timber

/**
 * Application class for ESP32 Sensor Logger
 * Phase B: BLE Communication
 * 
 * Initializes global application components and logging
 */
class SensorLoggerApplication : Application() {

    override fun onCreate() {
        super.onCreate()
        
        // Initialize Timber logging
        if (BuildConfig.DEBUG) {
            Timber.plant(Timber.DebugTree())
        }
        
        Timber.i("SensorLoggerApplication initialized - Phase B (BLE)")
    }
}

// Made with Bob
