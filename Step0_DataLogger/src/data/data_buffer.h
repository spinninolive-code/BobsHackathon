/**
 * @file data_buffer.h
 * @brief Circular buffer for accelerometer data
 * @version 1.0.0
 * @date 2026-05-03
 * 
 * Thread-safe circular buffer for storing accelerometer samples
 */

#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include "../config.h"

/**
 * @class DataBuffer
 * @brief Circular buffer for accelerometer samples
 */
class DataBuffer {
public:
    /**
     * @brief Constructor
     */
    DataBuffer();
    
    /**
     * @brief Initialize the buffer
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Write a sample to the buffer
     * @param sample Sample to write
     * @return true if successful, false if buffer full
     */
    bool write(const AccelSample& sample);
    
    /**
     * @brief Read a sample from the buffer
     * @param sample Pointer to store read sample
     * @return true if successful, false if buffer empty
     */
    bool read(AccelSample* sample);
    
    /**
     * @brief Read multiple samples from the buffer
     * @param samples Array to store read samples
     * @param count Maximum number of samples to read
     * @return Number of samples actually read
     */
    uint16_t readMultiple(AccelSample* samples, uint16_t count);
    
    /**
     * @brief Peek at a sample without removing it
     * @param sample Pointer to store peeked sample
     * @param offset Offset from read position (0 = next sample)
     * @return true if successful, false if not enough samples
     */
    bool peek(AccelSample* sample, uint16_t offset = 0);
    
    /**
     * @brief Get number of samples available to read
     * @return Number of samples in buffer
     */
    uint16_t available() const;
    
    /**
     * @brief Get free space in buffer
     * @return Number of free slots
     */
    uint16_t freeSpace() const;
    
    /**
     * @brief Check if buffer is empty
     * @return true if empty
     */
    bool isEmpty() const;
    
    /**
     * @brief Check if buffer is full
     * @return true if full
     */
    bool isFull() const;
    
    /**
     * @brief Get buffer usage percentage
     * @return Usage percentage (0-100)
     */
    uint8_t getUsagePercent() const;
    
    /**
     * @brief Clear the buffer
     */
    void clear();
    
    /**
     * @brief Get buffer capacity
     * @return Total buffer size
     */
    uint16_t getCapacity() const { return BUFFER_SIZE; }
    
    /**
     * @brief Get total samples written
     * @return Total write count
     */
    uint32_t getTotalWritten() const { return totalWritten; }
    
    /**
     * @brief Get total samples read
     * @return Total read count
     */
    uint32_t getTotalRead() const { return totalRead; }
    
    /**
     * @brief Get overflow count
     * @return Number of times buffer was full
     */
    uint32_t getOverflowCount() const { return overflowCount; }
    
    /**
     * @brief Get peak usage
     * @return Maximum number of samples in buffer
     */
    uint16_t getPeakUsage() const { return peakUsage; }
    
    /**
     * @brief Reset statistics
     */
    void resetStats();

private:
    // Buffer storage
    AccelSample buffer[BUFFER_SIZE];
    
    // Buffer pointers (volatile for ISR safety)
    volatile uint16_t writeIndex;
    volatile uint16_t readIndex;
    volatile uint16_t count;
    
    // Statistics
    uint32_t totalWritten;
    uint32_t totalRead;
    uint32_t overflowCount;
    uint16_t peakUsage;
    
    // Helper functions
    uint16_t nextIndex(uint16_t index) const;
};

#endif // DATA_BUFFER_H

// Made with Bob
