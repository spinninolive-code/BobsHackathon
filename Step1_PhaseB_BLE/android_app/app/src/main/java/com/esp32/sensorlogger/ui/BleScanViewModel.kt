package com.esp32.sensorlogger.ui

import android.bluetooth.BluetoothDevice
import android.bluetooth.le.ScanResult
import androidx.lifecycle.LiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.esp32.sensorlogger.ble.BleManager

/**
 * ViewModel for BleScanActivity
 * Manages BLE scanning and device connection
 */
class BleScanViewModel(private val bleManager: BleManager) : ViewModel() {

    // Scanning state
    val isScanning: LiveData<Boolean> = bleManager.scanResults.map { it.isNotEmpty() }
    
    // Scan results
    val scanResults: LiveData<List<ScanResult>> = bleManager.scanResults
    
    // Connection state
    val connectionState: LiveData<BleManager.ConnectionState> = bleManager.connectionState
    
    // Error messages
    val errorMessage: LiveData<String> = bleManager.errorMessage

    /**
     * Start BLE scan
     */
    fun startScan() {
        bleManager.startScan()
    }

    /**
     * Stop BLE scan
     */
    fun stopScan() {
        bleManager.stopScan()
    }

    /**
     * Connect to selected device
     */
    fun connectToDevice(device: BluetoothDevice) {
        stopScan()
        bleManager.connect(device)
    }

    override fun onCleared() {
        super.onCleared()
        stopScan()
    }
}

/**
 * Extension function to map LiveData
 */
private fun <X, Y> LiveData<X>.map(transform: (X) -> Y): LiveData<Y> {
    val result = androidx.lifecycle.MediatorLiveData<Y>()
    result.addSource(this) { x ->
        result.value = transform(x)
    }
    return result
}

/**
 * ViewModel Factory
 */
class BleScanViewModelFactory(
    private val bleManager: BleManager
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(BleScanViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return BleScanViewModel(bleManager) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

// Made with Bob
