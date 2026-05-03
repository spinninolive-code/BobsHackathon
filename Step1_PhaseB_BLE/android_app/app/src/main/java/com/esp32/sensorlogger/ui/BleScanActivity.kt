package com.esp32.sensorlogger.ui

import android.bluetooth.BluetoothDevice
import android.bluetooth.le.ScanResult
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.esp32.sensorlogger.R
import com.esp32.sensorlogger.ble.BleManager
import com.esp32.sensorlogger.databinding.ActivityBleScanBinding
import com.google.android.material.snackbar.Snackbar
import timber.log.Timber

/**
 * BLE Scan Activity
 * Scans for and displays available ESP32 devices
 */
class BleScanActivity : AppCompatActivity() {

    private lateinit var binding: ActivityBleScanBinding
    private lateinit var viewModel: BleScanViewModel
    private lateinit var adapter: BleDeviceAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        binding = ActivityBleScanBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.title = getString(R.string.scan_for_devices)
        
        // Initialize ViewModel
        val bleManager = BleManager(this)
        viewModel = ViewModelProvider(
            this,
            BleScanViewModelFactory(bleManager)
        )[BleScanViewModel::class.java]
        
        setupRecyclerView()
        setupUI()
        setupObservers()
        
        // Start scanning automatically
        viewModel.startScan()
    }

    private fun setupRecyclerView() {
        adapter = BleDeviceAdapter { device ->
            viewModel.connectToDevice(device)
        }
        
        binding.recyclerViewDevices.layoutManager = LinearLayoutManager(this)
        binding.recyclerViewDevices.adapter = adapter
    }

    private fun setupUI() {
        binding.btnScan.setOnClickListener {
            if (viewModel.isScanning.value == true) {
                viewModel.stopScan()
            } else {
                viewModel.startScan()
            }
        }
        
        binding.swipeRefresh.setOnRefreshListener {
            viewModel.startScan()
        }
    }

    private fun setupObservers() {
        // Scanning state
        viewModel.isScanning.observe(this) { isScanning ->
            binding.swipeRefresh.isRefreshing = isScanning
            binding.btnScan.text = if (isScanning) {
                getString(R.string.stop_scan)
            } else {
                getString(R.string.start_scan)
            }
            binding.progressBar.visibility = if (isScanning) View.VISIBLE else View.GONE
        }
        
        // Scan results
        viewModel.scanResults.observe(this) { results ->
            adapter.submitList(results)
            binding.tvNoDevices.visibility = if (results.isEmpty()) View.VISIBLE else View.GONE
        }
        
        // Connection state
        viewModel.connectionState.observe(this) { state ->
            when (state) {
                BleManager.ConnectionState.CONNECTING -> {
                    Snackbar.make(binding.root, "Connecting...", Snackbar.LENGTH_SHORT).show()
                }
                BleManager.ConnectionState.CONNECTED -> {
                    Snackbar.make(binding.root, "Connected!", Snackbar.LENGTH_SHORT).show()
                    finish() // Return to MainActivity
                }
                BleManager.ConnectionState.DISCONNECTED -> {
                    // Do nothing
                }
                BleManager.ConnectionState.DISCONNECTING -> {
                    // Do nothing
                }
            }
        }
        
        // Error messages
        viewModel.errorMessage.observe(this) { message ->
            if (message.isNotEmpty()) {
                Snackbar.make(binding.root, message, Snackbar.LENGTH_LONG).show()
            }
        }
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    override fun onDestroy() {
        super.onDestroy()
        viewModel.stopScan()
    }
}

/**
 * RecyclerView Adapter for BLE devices
 */
class BleDeviceAdapter(
    private val onDeviceClick: (BluetoothDevice) -> Unit
) : androidx.recyclerview.widget.ListAdapter<ScanResult, BleDeviceAdapter.ViewHolder>(
    object : androidx.recyclerview.widget.DiffUtil.ItemCallback<ScanResult>() {
        override fun areItemsTheSame(oldItem: ScanResult, newItem: ScanResult): Boolean {
            return oldItem.device.address == newItem.device.address
        }
        
        override fun areContentsTheSame(oldItem: ScanResult, newItem: ScanResult): Boolean {
            return oldItem.device.address == newItem.device.address &&
                   oldItem.rssi == newItem.rssi
        }
    }
) {
    
    class ViewHolder(val binding: com.esp32.sensorlogger.databinding.ItemBleDeviceBinding) :
        androidx.recyclerview.widget.RecyclerView.ViewHolder(binding.root)
    
    override fun onCreateViewHolder(parent: android.view.ViewGroup, viewType: Int): ViewHolder {
        val binding = com.esp32.sensorlogger.databinding.ItemBleDeviceBinding.inflate(
            android.view.LayoutInflater.from(parent.context),
            parent,
            false
        )
        return ViewHolder(binding)
    }
    
    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val result = getItem(position)
        val device = result.device
        
        holder.binding.tvDeviceName.text = device.name ?: "Unknown Device"
        holder.binding.tvDeviceAddress.text = device.address
        holder.binding.tvRssi.text = "${result.rssi} dBm"
        
        holder.binding.root.setOnClickListener {
            onDeviceClick(device)
        }
    }
}

// Made with Bob
