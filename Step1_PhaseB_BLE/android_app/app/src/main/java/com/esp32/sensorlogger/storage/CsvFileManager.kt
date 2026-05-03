package com.esp32.sensorlogger.storage

import android.content.Context
import android.os.Environment
import com.esp32.sensorlogger.data.SensorData
import com.opencsv.CSVWriter
import timber.log.Timber
import java.io.File
import java.io.FileWriter
import java.text.SimpleDateFormat
import java.util.*

/**
 * CSV File Manager for storing sensor data
 * Handles file creation, writing, and management
 * 
 * Storage locations:
 * - Internal storage: /data/data/com.esp32.sensorlogger/files/sensor_data/
 * - External storage (SD card): /storage/sdcard1/SensorLogger/
 * - Primary external: /storage/emulated/0/SensorLogger/
 */
class CsvFileManager(private val context: Context) {

    companion object {
        private const val INTERNAL_DIR = "sensor_data"
        private const val EXTERNAL_DIR = "SensorLogger"
        private const val FILE_PREFIX = "sensor_data_"
        private const val FILE_EXTENSION = ".csv"
    }

    private var currentFile: File? = null
    private var csvWriter: CSVWriter? = null
    private var recordCount = 0

    /**
     * Storage location enum
     */
    enum class StorageLocation {
        INTERNAL,           // Internal app storage
        EXTERNAL_PRIMARY,   // Primary external storage (usually internal SD)
        EXTERNAL_SD_CARD    // External SD card (if available)
    }

    /**
     * Get available storage locations
     */
    fun getAvailableStorageLocations(): List<StorageLocation> {
        val locations = mutableListOf<StorageLocation>()
        
        // Internal storage is always available
        locations.add(StorageLocation.INTERNAL)
        
        // Check primary external storage
        if (Environment.getExternalStorageState() == Environment.MEDIA_MOUNTED) {
            locations.add(StorageLocation.EXTERNAL_PRIMARY)
        }
        
        // Check for external SD card
        val externalDirs = context.getExternalFilesDirs(null)
        if (externalDirs.size > 1 && externalDirs[1] != null) {
            locations.add(StorageLocation.EXTERNAL_SD_CARD)
        }
        
        return locations
    }

    /**
     * Get storage directory for specified location
     */
    private fun getStorageDirectory(location: StorageLocation): File? {
        return when (location) {
            StorageLocation.INTERNAL -> {
                File(context.filesDir, INTERNAL_DIR)
            }
            StorageLocation.EXTERNAL_PRIMARY -> {
                if (Environment.getExternalStorageState() == Environment.MEDIA_MOUNTED) {
                    File(Environment.getExternalStorageDirectory(), EXTERNAL_DIR)
                } else {
                    null
                }
            }
            StorageLocation.EXTERNAL_SD_CARD -> {
                val externalDirs = context.getExternalFilesDirs(null)
                if (externalDirs.size > 1 && externalDirs[1] != null) {
                    File(externalDirs[1], EXTERNAL_DIR)
                } else {
                    null
                }
            }
        }
    }

    /**
     * Create new CSV file with timestamp-based name
     */
    fun createNewFile(location: StorageLocation = StorageLocation.INTERNAL): File? {
        closeCurrentFile()
        
        val directory = getStorageDirectory(location)
        if (directory == null) {
            Timber.e("Storage location not available: $location")
            return null
        }
        
        // Create directory if it doesn't exist
        if (!directory.exists()) {
            if (!directory.mkdirs()) {
                Timber.e("Failed to create directory: ${directory.absolutePath}")
                return null
            }
        }
        
        // Generate filename with timestamp
        val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault()).format(Date())
        val filename = "$FILE_PREFIX$timestamp$FILE_EXTENSION"
        val file = File(directory, filename)
        
        try {
            // Create file and write header
            val fileWriter = FileWriter(file, false)
            csvWriter = CSVWriter(fileWriter)
            
            // Write CSV header
            csvWriter?.writeNext(SensorData.CSV_HEADER.split(",").toTypedArray())
            csvWriter?.flush()
            
            currentFile = file
            recordCount = 0
            
            Timber.i("Created new CSV file: ${file.absolutePath}")
            return file
        } catch (e: Exception) {
            Timber.e(e, "Failed to create CSV file")
            return null
        }
    }

    /**
     * Write sensor data to current file
     */
    fun writeSensorData(data: SensorData): Boolean {
        if (csvWriter == null) {
            Timber.w("No file open for writing")
            return false
        }
        
        try {
            val row = data.toCsvRow().split(",").toTypedArray()
            csvWriter?.writeNext(row)
            recordCount++
            
            // Flush every 100 records to ensure data is written
            if (recordCount % 100 == 0) {
                csvWriter?.flush()
            }
            
            return true
        } catch (e: Exception) {
            Timber.e(e, "Failed to write sensor data")
            return false
        }
    }

    /**
     * Write multiple sensor data records
     */
    fun writeSensorDataBatch(dataList: List<SensorData>): Int {
        var successCount = 0
        dataList.forEach { data ->
            if (writeSensorData(data)) {
                successCount++
            }
        }
        return successCount
    }

    /**
     * Close current file
     */
    fun closeCurrentFile() {
        try {
            csvWriter?.flush()
            csvWriter?.close()
            csvWriter = null
            
            if (currentFile != null) {
                Timber.i("Closed CSV file: ${currentFile?.absolutePath}, records: $recordCount")
                currentFile = null
                recordCount = 0
            }
        } catch (e: Exception) {
            Timber.e(e, "Error closing CSV file")
        }
    }

    /**
     * Get current file info
     */
    fun getCurrentFileInfo(): FileInfo? {
        val file = currentFile ?: return null
        return FileInfo(
            path = file.absolutePath,
            name = file.name,
            size = file.length(),
            recordCount = recordCount,
            isOpen = csvWriter != null
        )
    }

    /**
     * List all CSV files in specified location
     */
    fun listFiles(location: StorageLocation): List<FileInfo> {
        val directory = getStorageDirectory(location) ?: return emptyList()
        
        if (!directory.exists()) {
            return emptyList()
        }
        
        val files = directory.listFiles { file ->
            file.isFile && file.name.endsWith(FILE_EXTENSION)
        } ?: return emptyList()
        
        return files.map { file ->
            FileInfo(
                path = file.absolutePath,
                name = file.name,
                size = file.length(),
                recordCount = countRecords(file),
                isOpen = false
            )
        }.sortedByDescending { it.name }
    }

    /**
     * Count records in a CSV file
     */
    private fun countRecords(file: File): Int {
        return try {
            file.useLines { lines ->
                lines.count() - 1 // Subtract header line
            }
        } catch (e: Exception) {
            0
        }
    }

    /**
     * Delete a file
     */
    fun deleteFile(filePath: String): Boolean {
        return try {
            val file = File(filePath)
            if (file.exists()) {
                file.delete()
            } else {
                false
            }
        } catch (e: Exception) {
            Timber.e(e, "Failed to delete file: $filePath")
            false
        }
    }

    /**
     * Get total storage usage
     */
    fun getStorageUsage(location: StorageLocation): StorageUsage {
        val directory = getStorageDirectory(location) ?: return StorageUsage(0, 0, 0)
        
        if (!directory.exists()) {
            return StorageUsage(0, 0, 0)
        }
        
        val files = directory.listFiles { file ->
            file.isFile && file.name.endsWith(FILE_EXTENSION)
        } ?: return StorageUsage(0, 0, 0)
        
        val totalSize = files.sumOf { it.length() }
        val fileCount = files.size
        val totalRecords = files.sumOf { countRecords(it) }
        
        return StorageUsage(totalSize, fileCount, totalRecords)
    }

    /**
     * File information data class
     */
    data class FileInfo(
        val path: String,
        val name: String,
        val size: Long,
        val recordCount: Int,
        val isOpen: Boolean
    ) {
        fun getSizeInMB(): String {
            return String.format("%.2f MB", size / (1024.0 * 1024.0))
        }
    }

    /**
     * Storage usage data class
     */
    data class StorageUsage(
        val totalSize: Long,
        val fileCount: Int,
        val totalRecords: Int
    ) {
        fun getSizeInMB(): String {
            return String.format("%.2f MB", totalSize / (1024.0 * 1024.0))
        }
    }
}

// Made with Bob
