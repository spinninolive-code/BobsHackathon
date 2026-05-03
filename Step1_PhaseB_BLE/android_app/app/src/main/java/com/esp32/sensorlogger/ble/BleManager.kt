package com.esp32.sensorlogger.ble

import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.esp32.sensorlogger.data.SensorData
import timber.log.Timber
import java.util.*

/**
 * BLE Manager for ESP32 Sensor Logger
 * Handles BLE scanning, connection, and data communication
 * 
 * Nordic UART Service (NUS) UUIDs:
 * - Service: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * - RX Characteristic (write): 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
 * - TX Characteristic (notify): 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
 * - Status Characteristic (read): 6E400004-B5A3-F393-E0A9-E50E24DCCA9E
 */
class BleManager(private val context: Context) {

    // Nordic UART Service UUIDs
    companion object {
        val NUS_SERVICE_UUID: UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
        val NUS_RX_CHAR_UUID: UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
        val NUS_TX_CHAR_UUID: UUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")
        val NUS_STATUS_CHAR_UUID: UUID = UUID.fromString("6E400004-B5A3-F393-E0A9-E50E24DCCA9E")
        
        val CLIENT_CHARACTERISTIC_CONFIG: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
        
        private const val SCAN_PERIOD: Long = 10000 // 10 seconds
    }

    private val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
    private val bluetoothLeScanner = bluetoothAdapter?.bluetoothLeScanner
    private val handler = Handler(Looper.getMainLooper())
    
    private var bluetoothGatt: BluetoothGatt? = null
    private var nusRxCharacteristic: BluetoothGattCharacteristic? = null
    private var nusTxCharacteristic: BluetoothGattCharacteristic? = null
    private var nusStatusCharacteristic: BluetoothGattCharacteristic? = null
    
    // LiveData for UI updates
    private val _connectionState = MutableLiveData<ConnectionState>(ConnectionState.DISCONNECTED)
    val connectionState: LiveData<ConnectionState> = _connectionState
    
    private val _scanResults = MutableLiveData<List<ScanResult>>()
    val scanResults: LiveData<List<ScanResult>> = _scanResults
    
    private val _sensorData = MutableLiveData<SensorData>()
    val sensorData: LiveData<SensorData> = _sensorData
    
    private val _deviceStatus = MutableLiveData<DeviceStatus>()
    val deviceStatus: LiveData<DeviceStatus> = _deviceStatus
    
    private val _errorMessage = MutableLiveData<String>()
    val errorMessage: LiveData<String> = _errorMessage
    
    private val scanResultsMap = mutableMapOf<String, ScanResult>()
    private var isScanning = false

    /**
     * Connection states
     */
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    }

    /**
     * Device status from ESP32
     */
    data class DeviceStatus(
        val state: String,
        val batteryVoltage: Float,
        val batteryPercent: Int,
        val sampleRate: Int,
        val bufferUsage: Int
    )

    /**
     * Start BLE scan for ESP32 devices
     */
    fun startScan() {
        if (isScanning) {
            Timber.w("Scan already in progress")
            return
        }
        
        if (bluetoothLeScanner == null) {
            _errorMessage.postValue("Bluetooth LE not supported")
            return
        }
        
        scanResultsMap.clear()
        _scanResults.postValue(emptyList())
        
        val scanFilter = ScanFilter.Builder()
            .setServiceUuid(android.os.ParcelUuid(NUS_SERVICE_UUID))
            .build()
        
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        
        isScanning = true
        bluetoothLeScanner.startScan(listOf(scanFilter), scanSettings, scanCallback)
        
        Timber.i("BLE scan started")
        
        // Stop scan after SCAN_PERIOD
        handler.postDelayed({
            stopScan()
        }, SCAN_PERIOD)
    }

    /**
     * Stop BLE scan
     */
    fun stopScan() {
        if (!isScanning) return
        
        isScanning = false
        bluetoothLeScanner?.stopScan(scanCallback)
        Timber.i("BLE scan stopped")
    }

    /**
     * Scan callback
     */
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val deviceAddress = result.device.address
            scanResultsMap[deviceAddress] = result
            _scanResults.postValue(scanResultsMap.values.toList())
        }

        override fun onScanFailed(errorCode: Int) {
            Timber.e("BLE scan failed with error code: $errorCode")
            _errorMessage.postValue("Scan failed: $errorCode")
            isScanning = false
        }
    }

    /**
     * Connect to BLE device
     */
    fun connect(device: BluetoothDevice) {
        if (_connectionState.value != ConnectionState.DISCONNECTED) {
            Timber.w("Already connected or connecting")
            return
        }
        
        _connectionState.postValue(ConnectionState.CONNECTING)
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
        Timber.i("Connecting to ${device.address}")
    }

    /**
     * Disconnect from BLE device
     */
    fun disconnect() {
        if (bluetoothGatt == null) return
        
        _connectionState.postValue(ConnectionState.DISCONNECTING)
        bluetoothGatt?.disconnect()
        Timber.i("Disconnecting from device")
    }

    /**
     * Close GATT connection
     */
    fun close() {
        bluetoothGatt?.close()
        bluetoothGatt = null
        nusRxCharacteristic = null
        nusTxCharacteristic = null
        nusStatusCharacteristic = null
        _connectionState.postValue(ConnectionState.DISCONNECTED)
    }

    /**
     * GATT callback
     */
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    Timber.i("Connected to GATT server")
                    _connectionState.postValue(ConnectionState.CONNECTED)
                    gatt.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    Timber.i("Disconnected from GATT server")
                    _connectionState.postValue(ConnectionState.DISCONNECTED)
                    close()
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val nusService = gatt.getService(NUS_SERVICE_UUID)
                if (nusService != null) {
                    nusRxCharacteristic = nusService.getCharacteristic(NUS_RX_CHAR_UUID)
                    nusTxCharacteristic = nusService.getCharacteristic(NUS_TX_CHAR_UUID)
                    nusStatusCharacteristic = nusService.getCharacteristic(NUS_STATUS_CHAR_UUID)
                    
                    // Enable notifications on TX characteristic
                    nusTxCharacteristic?.let { char ->
                        gatt.setCharacteristicNotification(char, true)
                        val descriptor = char.getDescriptor(CLIENT_CHARACTERISTIC_CONFIG)
                        descriptor?.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                        gatt.writeDescriptor(descriptor)
                    }
                    
                    Timber.i("NUS service discovered and configured")
                    
                    // Read initial status
                    readDeviceStatus()
                } else {
                    Timber.e("NUS service not found")
                    _errorMessage.postValue("NUS service not found on device")
                }
            } else {
                Timber.e("Service discovery failed: $status")
                _errorMessage.postValue("Service discovery failed")
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                when (characteristic.uuid) {
                    NUS_STATUS_CHAR_UUID -> {
                        parseDeviceStatus(characteristic.value)
                    }
                }
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            when (characteristic.uuid) {
                NUS_TX_CHAR_UUID -> {
                    // Parse sensor data from binary format
                    val data = SensorData.fromBinaryData(characteristic.value)
                    if (data != null) {
                        _sensorData.postValue(data)
                    } else {
                        Timber.w("Failed to parse sensor data")
                    }
                }
            }
        }
    }

    /**
     * Send command to ESP32
     */
    fun sendCommand(command: String): Boolean {
        val characteristic = nusRxCharacteristic
        if (characteristic == null || bluetoothGatt == null) {
            Timber.e("Cannot send command: not connected")
            return false
        }
        
        characteristic.value = command.toByteArray()
        val success = bluetoothGatt?.writeCharacteristic(characteristic) ?: false
        
        if (success) {
            Timber.i("Command sent: $command")
        } else {
            Timber.e("Failed to send command: $command")
        }
        
        return success
    }

    /**
     * Start data acquisition on ESP32
     */
    fun startAcquisition(): Boolean {
        return sendCommand("START")
    }

    /**
     * Stop data acquisition on ESP32
     */
    fun stopAcquisition(): Boolean {
        return sendCommand("STOP")
    }

    /**
     * Read device status from ESP32
     */
    fun readDeviceStatus() {
        val characteristic = nusStatusCharacteristic
        if (characteristic != null && bluetoothGatt != null) {
            bluetoothGatt?.readCharacteristic(characteristic)
        }
    }

    /**
     * Parse device status from binary data
     * Format: state(1),voltage(4),percent(1),rate(2),buffer(1)
     */
    private fun parseDeviceStatus(data: ByteArray) {
        if (data.size < 9) return
        
        try {
            val state = when (data[0].toInt()) {
                0 -> "IDLE"
                1 -> "ACQUIRING"
                2 -> "STORING"
                3 -> "ERROR"
                else -> "UNKNOWN"
            }
            
            val voltage = Float.fromBits(
                (data[1].toInt() and 0xFF) or
                ((data[2].toInt() and 0xFF) shl 8) or
                ((data[3].toInt() and 0xFF) shl 16) or
                ((data[4].toInt() and 0xFF) shl 24)
            )
            
            val percent = data[5].toInt() and 0xFF
            
            val rate = ((data[6].toInt() and 0xFF) or
                       ((data[7].toInt() and 0xFF) shl 8))
            
            val buffer = data[8].toInt() and 0xFF
            
            val status = DeviceStatus(state, voltage, percent, rate, buffer)
            _deviceStatus.postValue(status)
            
            Timber.d("Device status: $status")
        } catch (e: Exception) {
            Timber.e(e, "Failed to parse device status")
        }
    }
}

// Made with Bob
