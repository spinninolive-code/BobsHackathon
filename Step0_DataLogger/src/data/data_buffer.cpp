/**
 * @file data_buffer.cpp
 * @brief Circular buffer implementation
 * @version 1.0.0
 * @date 2026-05-03
 */

#include "data_buffer.h"

DataBuffer::DataBuffer()
    : writeIndex(0)
    , readIndex(0)
    , count(0)
    , totalWritten(0)
    , totalRead(0)
    , overflowCount(0)
    , peakUsage(0)
{
}

bool DataBuffer::begin() {
    clear();
    DEBUG_PRINTF("DataBuffer initialized: %d samples capacity\n", BUFFER_SIZE);
    return true;
}

bool DataBuffer::write(const AccelSample& sample) {
    // Check if buffer is full
    if (count >= BUFFER_SIZE) {
        overflowCount++;
        return false;
    }
    
    // Disable interrupts for atomic operation
    noInterrupts();
    
    // Write sample
    buffer[writeIndex] = sample;
    writeIndex = nextIndex(writeIndex);
    count++;
    totalWritten++;
    
    // Update peak usage
    if (count > peakUsage) {
        peakUsage = count;
    }
    
    interrupts();
    
    return true;
}

bool DataBuffer::read(AccelSample* sample) {
    // Check if buffer is empty
    if (count == 0) {
        return false;
    }
    
    // Disable interrupts for atomic operation
    noInterrupts();
    
    // Read sample
    *sample = buffer[readIndex];
    readIndex = nextIndex(readIndex);
    count--;
    totalRead++;
    
    interrupts();
    
    return true;
}

uint16_t DataBuffer::readMultiple(AccelSample* samples, uint16_t maxCount) {
    uint16_t samplesRead = 0;
    
    // Disable interrupts for atomic operation
    noInterrupts();
    
    // Read up to maxCount samples or until buffer empty
    while (samplesRead < maxCount && count > 0) {
        samples[samplesRead] = buffer[readIndex];
        readIndex = nextIndex(readIndex);
        count--;
        totalRead++;
        samplesRead++;
    }
    
    interrupts();
    
    return samplesRead;
}

bool DataBuffer::peek(AccelSample* sample, uint16_t offset) {
    // Check if enough samples available
    if (offset >= count) {
        return false;
    }
    
    // Calculate peek index
    uint16_t peekIndex = (readIndex + offset) % BUFFER_SIZE;
    
    // Read sample without modifying pointers
    noInterrupts();
    *sample = buffer[peekIndex];
    interrupts();
    
    return true;
}

uint16_t DataBuffer::available() const {
    return count;
}

uint16_t DataBuffer::freeSpace() const {
    return BUFFER_SIZE - count;
}

bool DataBuffer::isEmpty() const {
    return (count == 0);
}

bool DataBuffer::isFull() const {
    return (count >= BUFFER_SIZE);
}

uint8_t DataBuffer::getUsagePercent() const {
    return (count * 100) / BUFFER_SIZE;
}

void DataBuffer::clear() {
    noInterrupts();
    writeIndex = 0;
    readIndex = 0;
    count = 0;
    interrupts();
    
    DEBUG_PRINTLN("DataBuffer cleared");
}

void DataBuffer::resetStats() {
    noInterrupts();
    totalWritten = 0;
    totalRead = 0;
    overflowCount = 0;
    peakUsage = count;
    interrupts();
    
    DEBUG_PRINTLN("DataBuffer statistics reset");
}

uint16_t DataBuffer::nextIndex(uint16_t index) const {
    return (index + 1) % BUFFER_SIZE;
}

// Made with Bob
