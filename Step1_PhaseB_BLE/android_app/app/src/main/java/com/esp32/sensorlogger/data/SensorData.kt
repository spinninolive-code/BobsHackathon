package com.esp32.sensorlogger.data

import java.text.SimpleDateFormat
import java.util.*

/**
 * Data class representing a single sensor reading from ESP32
 * Contains data from all three sensors: ICM-42688-P, MMC5983MA, LPS22HB
 */
data class SensorData(
    val timestamp: Long = System.currentTimeMillis(),
    
    // ICM-42688-P Accelerometer (±16g)
    val accelX: Float = 0f,
    val accelY: Float = 0f,
    val accelZ: Float = 0f,
    
    // ICM-42688-P Gyroscope (±2000 dps)
    val gyroX: Float = 0f,
    val gyroY: Float = 0f,
    val gyroZ: Float = 0f,
    
    // MMC5983MA Magnetometer
    val magX: Float = 0f,
    val magY: Float = 0f,
    val magZ: Float = 0f,
    
    // LPS22HB Pressure and Temperature
    val pressure: Float = 0f,      // hPa
    val temperature: Float = 0f    // °C
) {
    /**
     * Convert to CSV row format
     * Format: timestamp,accelX,accelY,accelZ,gyroX,gyroY,gyroZ,magX,magY,magZ,pressure,temperature
     */
    fun toCsvRow(): String {
        return "$timestamp,$accelX,$accelY,$accelZ,$gyroX,$gyroY,$gyroZ,$magX,$magY,$magZ,$pressure,$temperature"
    }
    
    /**
     * Get formatted timestamp string
     */
    fun getFormattedTimestamp(): String {
        val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault())
        return sdf.format(Date(timestamp))
    }
    
    companion object {
        /**
         * CSV header for sensor data files
         */
        const val CSV_HEADER = "timestamp,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z,pressure,temperature"
        
        /**
         * Parse sensor data from binary format received via BLE
         * Binary format (52 bytes):
         * - timestamp: 8 bytes (long)
         * - accel_x, accel_y, accel_z: 12 bytes (3 floats)
         * - gyro_x, gyro_y, gyro_z: 12 bytes (3 floats)
         * - mag_x, mag_y, mag_z: 12 bytes (3 floats)
         * - pressure: 4 bytes (float)
         * - temperature: 4 bytes (float)
         */
        fun fromBinaryData(data: ByteArray): SensorData? {
            if (data.size < 52) return null
            
            try {
                var offset = 0
                
                // Parse timestamp (8 bytes, little-endian)
                val timestamp = data.getLongLE(offset)
                offset += 8
                
                // Parse accelerometer (12 bytes)
                val accelX = data.getFloatLE(offset)
                offset += 4
                val accelY = data.getFloatLE(offset)
                offset += 4
                val accelZ = data.getFloatLE(offset)
                offset += 4
                
                // Parse gyroscope (12 bytes)
                val gyroX = data.getFloatLE(offset)
                offset += 4
                val gyroY = data.getFloatLE(offset)
                offset += 4
                val gyroZ = data.getFloatLE(offset)
                offset += 4
                
                // Parse magnetometer (12 bytes)
                val magX = data.getFloatLE(offset)
                offset += 4
                val magY = data.getFloatLE(offset)
                offset += 4
                val magZ = data.getFloatLE(offset)
                offset += 4
                
                // Parse pressure (4 bytes)
                val pressure = data.getFloatLE(offset)
                offset += 4
                
                // Parse temperature (4 bytes)
                val temperature = data.getFloatLE(offset)
                
                return SensorData(
                    timestamp = timestamp,
                    accelX = accelX,
                    accelY = accelY,
                    accelZ = accelZ,
                    gyroX = gyroX,
                    gyroY = gyroY,
                    gyroZ = gyroZ,
                    magX = magX,
                    magY = magY,
                    magZ = magZ,
                    pressure = pressure,
                    temperature = temperature
                )
            } catch (e: Exception) {
                return null
            }
        }
        
        /**
         * Parse sensor data from CSV row
         */
        fun fromCsvRow(row: String): SensorData? {
            try {
                val values = row.split(",")
                if (values.size < 12) return null
                
                return SensorData(
                    timestamp = values[0].toLong(),
                    accelX = values[1].toFloat(),
                    accelY = values[2].toFloat(),
                    accelZ = values[3].toFloat(),
                    gyroX = values[4].toFloat(),
                    gyroY = values[5].toFloat(),
                    gyroZ = values[6].toFloat(),
                    magX = values[7].toFloat(),
                    magY = values[8].toFloat(),
                    magZ = values[9].toFloat(),
                    pressure = values[10].toFloat(),
                    temperature = values[11].toFloat()
                )
            } catch (e: Exception) {
                return null
            }
        }
    }
}

/**
 * Extension function to read little-endian long from ByteArray
 */
private fun ByteArray.getLongLE(offset: Int): Long {
    return (this[offset].toLong() and 0xFF) or
           ((this[offset + 1].toLong() and 0xFF) shl 8) or
           ((this[offset + 2].toLong() and 0xFF) shl 16) or
           ((this[offset + 3].toLong() and 0xFF) shl 24) or
           ((this[offset + 4].toLong() and 0xFF) shl 32) or
           ((this[offset + 5].toLong() and 0xFF) shl 40) or
           ((this[offset + 6].toLong() and 0xFF) shl 48) or
           ((this[offset + 7].toLong() and 0xFF) shl 56)
}

/**
 * Extension function to read little-endian float from ByteArray
 */
private fun ByteArray.getFloatLE(offset: Int): Float {
    val bits = (this[offset].toInt() and 0xFF) or
               ((this[offset + 1].toInt() and 0xFF) shl 8) or
               ((this[offset + 2].toInt() and 0xFF) shl 16) or
               ((this[offset + 3].toInt() and 0xFF) shl 24)
    return Float.fromBits(bits)
}

// Made with Bob
