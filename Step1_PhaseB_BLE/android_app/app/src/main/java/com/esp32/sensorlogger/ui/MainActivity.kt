package com.esp32.sensorlogger.ui

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.lifecycle.ViewModelProvider
import com.esp32.sensorlogger.R
import com.esp32.sensorlogger.ble.BleManager
import com.esp32.sensorlogger.databinding.ActivityMainBinding
import com.esp32.sensorlogger.storage.CsvFileManager
import com.google.android.material.snackbar.Snackbar
import com.karumi.dexter.Dexter
import com.karumi.dexter.MultiplePermissionsReport
import com.karumi.dexter.PermissionToken
import com.karumi.dexter.listener.PermissionRequest
import com.karumi.dexter.listener.multi.MultiplePermissionsListener
import timber.log.Timber

/**
 * Main Activity for ESP32 Sensor Logger
 * Phase B: BLE Communication
 * 
 * Features:
 * - BLE device connection management
 * - Start/stop data acquisition
 * - Real-time data display
 * - CSV file storage
 * - Device status monitoring
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var viewModel: MainViewModel
    private lateinit var bleManager: BleManager
    private lateinit var csvFileManager: CsvFileManager

    companion object {
        private const val REQUEST_ENABLE_BT = 1
        private const val REQUEST_PERMISSIONS = 2
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setSupportActionBar(binding.toolbar)
        
        // Initialize managers
        bleManager = BleManager(this)
        csvFileManager = CsvFileManager(this)
        
        // Initialize ViewModel
        viewModel = ViewModelProvider(
            this,
            MainViewModelFactory(bleManager, csvFileManager)
        )[MainViewModel::class.java]
        
        setupUI()
        setupObservers()
        checkPermissions()
    }

    private fun setupUI() {
        // Connect button
        binding.btnConnect.setOnClickListener {
            if (viewModel.isConnected.value == true) {
                viewModel.disconnect()
            } else {
                startActivity(Intent(this, BleScanActivity::class.java))
            }
        }
        
        // Start/Stop acquisition button
        binding.btnStartStop.setOnClickListener {
            if (viewModel.isAcquiring.value == true) {
                viewModel.stopAcquisition()
            } else {
                viewModel.startAcquisition()
            }
        }
        
        // View data button
        binding.btnViewData.setOnClickListener {
            startActivity(Intent(this, DataViewerActivity::class.java))
        }
        
        // Storage location spinner
        binding.spinnerStorageLocation.adapter = android.widget.ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            csvFileManager.getAvailableStorageLocations().map { it.name }
        ).apply {
            setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        }
        
        binding.spinnerStorageLocation.onItemSelectedListener = 
            object : android.widget.AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: android.widget.AdapterView<*>?,
                    view: android.view.View?,
                    position: Int,
                    id: Long
                ) {
                    val location = csvFileManager.getAvailableStorageLocations()[position]
                    viewModel.setStorageLocation(location)
                }
                
                override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {}
            }
    }

    private fun setupObservers() {
        // Connection state
        viewModel.isConnected.observe(this) { isConnected ->
            binding.btnConnect.text = if (isConnected) {
                getString(R.string.disconnect)
            } else {
                getString(R.string.connect)
            }
            binding.btnStartStop.isEnabled = isConnected
            binding.tvConnectionStatus.text = if (isConnected) {
                getString(R.string.connected)
            } else {
                getString(R.string.disconnected)
            }
        }
        
        // Acquisition state
        viewModel.isAcquiring.observe(this) { isAcquiring ->
            binding.btnStartStop.text = if (isAcquiring) {
                getString(R.string.stop_acquisition)
            } else {
                getString(R.string.start_acquisition)
            }
            binding.tvAcquisitionStatus.text = if (isAcquiring) {
                getString(R.string.acquiring)
            } else {
                getString(R.string.idle)
            }
        }
        
        // Sensor data
        viewModel.sensorData.observe(this) { data ->
            binding.tvAccelX.text = String.format("%.3f g", data.accelX)
            binding.tvAccelY.text = String.format("%.3f g", data.accelY)
            binding.tvAccelZ.text = String.format("%.3f g", data.accelZ)
            
            binding.tvGyroX.text = String.format("%.2f °/s", data.gyroX)
            binding.tvGyroY.text = String.format("%.2f °/s", data.gyroY)
            binding.tvGyroZ.text = String.format("%.2f °/s", data.gyroZ)
            
            binding.tvMagX.text = String.format("%.2f µT", data.magX)
            binding.tvMagY.text = String.format("%.2f µT", data.magY)
            binding.tvMagZ.text = String.format("%.2f µT", data.magZ)
            
            binding.tvPressure.text = String.format("%.2f hPa", data.pressure)
            binding.tvTemperature.text = String.format("%.2f °C", data.temperature)
        }
        
        // Device status
        viewModel.deviceStatus.observe(this) { status ->
            binding.tvDeviceState.text = status.state
            binding.tvBatteryVoltage.text = String.format("%.2f V", status.batteryVoltage)
            binding.tvBatteryPercent.text = String.format("%d%%", status.batteryPercent)
            binding.tvSampleRate.text = String.format("%d Hz", status.sampleRate)
            binding.tvBufferUsage.text = String.format("%d%%", status.bufferUsage)
            
            // Update battery progress bar
            binding.progressBattery.progress = status.batteryPercent
        }
        
        // Record count
        viewModel.recordCount.observe(this) { count ->
            binding.tvRecordCount.text = String.format("%d records", count)
        }
        
        // File info
        viewModel.currentFileInfo.observe(this) { fileInfo ->
            if (fileInfo != null) {
                binding.tvCurrentFile.text = fileInfo.name
                binding.tvFileSize.text = fileInfo.getSizeInMB()
            } else {
                binding.tvCurrentFile.text = getString(R.string.no_file)
                binding.tvFileSize.text = "0.00 MB"
            }
        }
        
        // Error messages
        viewModel.errorMessage.observe(this) { message ->
            if (message.isNotEmpty()) {
                Snackbar.make(binding.root, message, Snackbar.LENGTH_LONG).show()
            }
        }
    }

    private fun checkPermissions() {
        val permissions = mutableListOf<String>()
        
        // BLE permissions
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            permissions.add(Manifest.permission.BLUETOOTH)
            permissions.add(Manifest.permission.BLUETOOTH_ADMIN)
        }
        
        // Location permissions (required for BLE scanning)
        permissions.add(Manifest.permission.ACCESS_FINE_LOCATION)
        
        // Storage permissions
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE)
            permissions.add(Manifest.permission.READ_EXTERNAL_STORAGE)
        }
        
        Dexter.withContext(this)
            .withPermissions(permissions)
            .withListener(object : MultiplePermissionsListener {
                override fun onPermissionsChecked(report: MultiplePermissionsReport) {
                    if (report.areAllPermissionsGranted()) {
                        checkBluetoothEnabled()
                    } else {
                        Snackbar.make(
                            binding.root,
                            "Permissions required for BLE and storage",
                            Snackbar.LENGTH_LONG
                        ).show()
                    }
                }
                
                override fun onPermissionRationaleShouldBeShown(
                    permissions: List<PermissionRequest>,
                    token: PermissionToken
                ) {
                    token.continuePermissionRequest()
                }
            })
            .check()
    }

    private fun checkBluetoothEnabled() {
        val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        if (bluetoothAdapter == null) {
            Snackbar.make(
                binding.root,
                "Bluetooth not supported on this device",
                Snackbar.LENGTH_LONG
            ).show()
            return
        }
        
        if (!bluetoothAdapter.isEnabled) {
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.BLUETOOTH_CONNECT
                ) == PackageManager.PERMISSION_GRANTED
            ) {
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
            }
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_settings -> {
                startActivity(Intent(this, SettingsActivity::class.java))
                true
            }
            R.id.action_refresh_status -> {
                viewModel.refreshDeviceStatus()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        viewModel.cleanup()
    }
}

// Made with Bob
