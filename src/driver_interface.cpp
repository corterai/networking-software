#include "driver_interface.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>

DriverInterface::DriverInterface(const std::string& device_path)
    : type_(Type::PCIE)
    , device_path_(device_path)
    , driver_version_("1.0.0")
    , is_open_(false)
    , interrupt_running_(false) {
}

DriverInterface::~DriverInterface() {
    if (is_open_) {
        close();
    }
    cleanupMemoryMappings();
}

DriverInterface::Type DriverInterface::getType() const {
    return type_;
}

std::string DriverInterface::getDevicePath() const {
    return device_path_;
}

std::string DriverInterface::getDriverVersion() const {
    return driver_version_;
}

bool DriverInterface::isOpen() const {
    return is_open_;
}

void DriverInterface::setLastError(const std::string& error) const {
    last_error_ = error;
}

bool DriverInterface::validateOffset(uint64_t offset, size_t size, size_t max_size) const {
    return offset + size <= max_size;
}

bool DriverInterface::validateBuffer(const void* buffer, size_t size) const {
    return buffer != nullptr && size > 0;
}

bool DriverInterface::mapMemoryRegion(uint64_t physical_addr, size_t size, void** virtual_addr) {
    // This is a simplified implementation
    // In a real driver, this would use mmap() or similar system calls
    *virtual_addr = reinterpret_cast<void*>(physical_addr);
    return true;
}

void DriverInterface::unmapMemoryRegion(void* virtual_addr, size_t size) {
    // This is a simplified implementation
    // In a real driver, this would use munmap() or similar system calls
    (void)virtual_addr;
    (void)size;
}

bool DriverInterface::readRegisterArray(uint32_t base_offset, uint32_t* values, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!readRegister(base_offset + i * 4, values[i])) {
            return false;
        }
    }
    return true;
}

bool DriverInterface::writeRegisterArray(uint32_t base_offset, const uint32_t* values, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!writeRegister(base_offset + i * 4, values[i])) {
            return false;
        }
    }
    return true;
}

bool DriverInterface::allocateAlignedBuffer(size_t size, size_t alignment, void** virtual_addr, uint64_t* physical_addr) {
    // This is a simplified implementation
    // In a real driver, this would use aligned_alloc() or similar
    *virtual_addr = std::aligned_alloc(alignment, size);
    if (*virtual_addr) {
        *physical_addr = reinterpret_cast<uint64_t>(*virtual_addr);
        return true;
    }
    return false;
}

void DriverInterface::freeAlignedBuffer(void* virtual_addr, size_t size) {
    // This is a simplified implementation
    // In a real driver, this would use free() or similar
    std::free(virtual_addr);
    (void)size;
}

bool DriverInterface::setupInterruptThread() {
    if (interrupt_running_) {
        return false;
    }
    
    interrupt_running_ = true;
    interrupt_thread_ = std::thread(&DriverInterface::interruptThreadFunction, this);
    return true;
}

void DriverInterface::interruptThreadFunction() {
    while (interrupt_running_) {
        // This is a simplified implementation
        // In a real driver, this would wait for actual hardware interrupts
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void DriverInterface::handleInterrupt(InterruptType type) {
    if (type < InterruptType::RX_COMPLETE || type > InterruptType::MANAGEMENT) {
        return;
    }
    
    size_t index = static_cast<size_t>(type);
    if (index < interrupt_handlers_.size() && interrupt_handlers_[index]) {
        interrupt_handlers_[index](type);
    }
}

bool DriverInterface::detectHardware() {
    // This is a simplified implementation
    // In a real driver, this would detect the actual hardware
    return true;
}

bool DriverInterface::initializeHardware() {
    // This is a simplified implementation
    // In a real driver, this would initialize the actual hardware
    return true;
}

bool DriverInterface::setupInterrupts() {
    // This is a simplified implementation
    // In a real driver, this would setup actual hardware interrupts
    return true;
}

bool DriverInterface::setupDMA() {
    // This is a simplified implementation
    // In a real driver, this would setup actual DMA
    return true;
}

void DriverInterface::cleanupHardware() {
    // This is a simplified implementation
    // In a real driver, this would cleanup actual hardware
}

void DriverInterface::cleanupMemoryMappings() {
    for (auto& mapping : memory_mappings_) {
        unmapMemoryRegion(mapping.virtual_addr, mapping.size);
    }
    memory_mappings_.clear();
}

bool DriverInterface::findMemoryMapping(uint64_t physical_addr, size_t size, MemoryMapping& mapping) {
    for (const auto& existing_mapping : memory_mappings_) {
        if (existing_mapping.physical_addr == physical_addr && existing_mapping.size == size) {
            mapping = existing_mapping;
            return true;
        }
    }
    return false;
}