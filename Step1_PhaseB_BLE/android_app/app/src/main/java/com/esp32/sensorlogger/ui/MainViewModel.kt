package com.esp32.sensorlogger.ui

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.esp32.sensorlogger.ble.BleManager
import com.esp32.sensorlogger.data.SensorData
import com.esp32.sensorlogger.storage.CsvFileManager
import timber.log.Timber

/**
 * ViewModel for MainActivity
 * Manages BLE connection, data acquisition, and file storage
 */
class MainViewModel(
    private val bleManager: BleManager,
    private val csvFileManager: CsvFileManager
) : ViewModel() {

    // Connection state
    private val _isConnected = MutableLiveData<Boolean>(false)
    val isConnected: LiveData<Boolean> = _isConnected
    
    // Acquisition state
    private val _isAcquiring = MutableLiveData<Boolean>(false)
    val isAcquiring: LiveData<Boolean> = _isAcquiring
    
    // Sensor data
    private val _sensorData = MutableLiveData<SensorData>()
    val sensorData: LiveData<SensorData> = _sensorData
    
    // Device status
    private val _deviceStatus = MutableLiveData<BleManager.DeviceStatus>()
    val deviceStatus: LiveData<BleManager.DeviceStatus> = _deviceStatus
    
    // Record count
    private val _recordCount = MutableLiveData<Int>(0)
    val recordCount: LiveData<Int> = _recordCount
    
    // Current file info
    private val _currentFileInfo = MutableLiveData<CsvFileManager.FileInfo?>()
    val currentFileInfo: LiveData<CsvFileManager.FileInfo?> = _currentFileInfo
    
    // Error messages
    private val _errorMessage = MutableLiveData<String>()
    val errorMessage: LiveData<String> = _errorMessage
    
    // Storage location
    private var currentStorageLocation = CsvFileManager.StorageLocation.INTERNAL

    init {
        // Observe BLE manager states
        bleManager.connectionState.observeForever { state ->
            _isConnected.postValue(state == BleManager.ConnectionState.CONNECTED)
        }
        
        bleManager.sensorData.observeForever { data ->
            _sensorData.postValue(data)
            
            // Write to file if acquiring
            if (_isAcquiring.value == true) {
                if (csvFileManager.writeSensorData(data)) {
                    _recordCount.postValue((_recordCount.value ?: 0) + 1)
                    updateFileInfo()
                }
            }
        }
        
        bleManager.deviceStatus.observeForever { status ->
            _deviceStatus.postValue(status)
        }
        
        bleManager.errorMessage.observeForever { message ->
            _errorMessage.postValue(message)
        }
    }

    /**
     * Disconnect from BLE device
     */
    fun disconnect() {
        if (_isAcquiring.value == true) {
            stopAcquisition()
        }
        bleManager.disconnect()
    }

    /**
     * Start data acquisition
     */
    fun startAcquisition() {
        if (_isConnected.value != true) {
            _errorMessage.postValue("Not connected to device")
            return
        }
        
        // Create new CSV file
        val file = csvFileManager.createNewFile(currentStorageLocation)
        if (file == null) {
            _errorMessage.postValue("Failed to create CSV file")
            return
        }
        
        // Send start command to ESP32
        if (bleManager.startAcquisition()) {
            _isAcquiring.postValue(true)
            _recordCount.postValue(0)
            updateFileInfo()
            Timber.i("Data acquisition started")
        } else {
            _errorMessage.postValue("Failed to start acquisition")
            csvFileManager.closeCurrentFile()
        }
    }

    /**
     * Stop data acquisition
     */
    fun stopAcquisition() {
        if (_isAcquiring.value != true) {
            return
        }
        
        // Send stop command to ESP32
        bleManager.stopAcquisition()
        
        // Close CSV file
        csvFileManager.closeCurrentFile()
        
        _isAcquiring.postValue(false)
        updateFileInfo()
        
        Timber.i("Data acquisition stopped, records: ${_recordCount.value}")
    }

    /**
     * Set storage location
     */
    fun setStorageLocation(location: CsvFileManager.StorageLocation) {
        if (_isAcquiring.value == true) {
            _errorMessage.postValue("Cannot change storage location during acquisition")
            return
        }
        currentStorageLocation = location
        Timber.i("Storage location set to: $location")
    }

    /**
     * Refresh device status
     */
    fun refreshDeviceStatus() {
        if (_isConnected.value == true) {
            bleManager.readDeviceStatus()
        }
    }

    /**
     * Update file info
     */
    private fun updateFileInfo() {
        _currentFileInfo.postValue(csvFileManager.getCurrentFileInfo())
    }

    /**
     * Cleanup resources
     */
    fun cleanup() {
        if (_isAcquiring.value == true) {
            stopAcquisition()
        }
        bleManager.close()
    }

    override fun onCleared() {
        super.onCleared()
        cleanup()
    }
}

/**
 * ViewModel Factory
 */
class MainViewModelFactory(
    private val bleManager: BleManager,
    private val csvFileManager: CsvFileManager
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return MainViewModel(bleManager, csvFileManager) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

// Made with Bob
