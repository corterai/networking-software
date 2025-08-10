#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class PacketBuffer;

/**
 * @brief Abstract driver interface for Ethernet NIC hardware
 * 
 * This class provides a hardware-agnostic interface for accessing
 * Ethernet controller registers, memory, and interrupts.
 */
class DriverInterface {
public:
    // Driver types
    enum class Type {
        PCIE,           // PCI Express
        PCI,            // PCI
        USB,            // USB
        VIRTUAL         // Virtual/emulated
    };

    // Memory types
    enum class MemoryType {
        REGISTER,       // Control registers
        EEPROM,         // EEPROM memory
        SRAM,           // Static RAM
        DRAM,           // Dynamic RAM
        SHARED          // Shared memory
    };

    // Interrupt types
    enum class InterruptType {
        RX_COMPLETE,    // Receive complete
        TX_COMPLETE,    // Transmit complete
        LINK_STATUS,    // Link status change
        ERROR,          // Error condition
        TIMER,          // Timer interrupt
        MANAGEMENT      // Management interrupt
    };

    // DMA descriptor structure
    struct DMADescriptor {
        uint64_t buffer_address;    // Physical buffer address
        uint32_t buffer_length;     // Buffer length
        uint32_t flags;             // Descriptor flags
        uint32_t status;            // Status information
        uint64_t next_descriptor;   // Next descriptor address
    };

    // Register access structure
    struct RegisterAccess {
        uint32_t offset;            // Register offset
        uint32_t value;             // Register value
        uint32_t mask;              // Write mask
        bool is_read;               // Read operation flag
    };

    // Constructor
    explicit DriverInterface(const std::string& device_path);
    
    // Destructor
    virtual ~DriverInterface();

    // Driver information
    Type getType() const;
    std::string getDevicePath() const;
    std::string getDriverVersion() const;
    bool isOpen() const;

    // Device management
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isReady() const = 0;
    virtual bool reset() = 0;

    // Memory access
    virtual bool readMemory(MemoryType type, uint64_t offset, void* buffer, size_t size) = 0;
    virtual bool writeMemory(MemoryType type, uint64_t offset, const void* buffer, size_t size) = 0;
    
    // Register access
    virtual bool readRegister(uint32_t offset, uint32_t& value) = 0;
    virtual bool writeRegister(uint32_t offset, uint32_t value) = 0;
    virtual bool readRegisterMasked(uint32_t offset, uint32_t& value, uint32_t mask) = 0;
    virtual bool writeRegisterMasked(uint32_t offset, uint32_t value, uint32_t mask) = 0;

    // DMA operations
    virtual bool allocateDMABuffer(size_t size, void** virtual_addr, uint64_t* physical_addr) = 0;
    virtual void freeDMABuffer(void* virtual_addr, size_t size) = 0;
    virtual bool mapDMABuffer(void* virtual_addr, size_t size, uint64_t* physical_addr) = 0;
    virtual void unmapDMABuffer(void* virtual_addr, size_t size) = 0;
    
    // DMA descriptor management
    virtual bool setupDMADescriptors(std::vector<DMADescriptor>& descriptors, 
                                   const std::vector<void*>& buffers, 
                                   const std::vector<size_t>& sizes) = 0;
    virtual bool updateDMADescriptor(uint32_t index, const DMADescriptor& descriptor) = 0;
    virtual bool getDMADescriptorStatus(uint32_t index, uint32_t& status) = 0;

    // Interrupt handling
    virtual bool enableInterrupts() = 0;
    virtual bool disableInterrupts() = 0;
    virtual bool registerInterruptHandler(InterruptType type, 
                                        std::function<void(InterruptType)> handler) = 0;
    virtual bool unregisterInterruptHandler(InterruptType type) = 0;
    virtual bool waitForInterrupt(InterruptType type, int timeout_ms = -1) = 0;
    virtual bool acknowledgeInterrupt(InterruptType type) = 0;

    // Device-specific operations
    virtual bool getDeviceInfo(std::string& vendor, std::string& device, 
                             std::string& revision) = 0;
    virtual bool getMACAddress(uint8_t* mac_address) = 0;
    virtual bool setMACAddress(const uint8_t* mac_address) = 0;
    virtual bool getLinkStatus(bool& link_up, uint32_t& speed, bool& duplex) = 0;
    virtual bool setLinkSpeed(uint32_t speed, bool duplex) = 0;

    // Power management
    virtual bool enterLowPowerMode() = 0;
    virtual bool exitLowPowerMode() = 0;
    virtual bool isLowPowerMode() const = 0;

    // Statistics
    virtual bool getHardwareStatistics(uint64_t* stats_array, size_t count) = 0;
    virtual bool resetHardwareStatistics() = 0;

    // Error handling
    virtual std::string getLastError() const = 0;
    virtual void clearLastError() = 0;
    virtual bool hasError() const = 0;

protected:
    // Common properties
    Type type_;
    std::string device_path_;
    std::string driver_version_;
    bool is_open_;
    
    // Error state
    mutable std::string last_error_;
    
    // Helper methods
    void setLastError(const std::string& error) const;
    bool validateOffset(uint64_t offset, size_t size, size_t max_size) const;
    bool validateBuffer(const void* buffer, size_t size) const;
    
    // Memory mapping helpers
    bool mapMemoryRegion(uint64_t physical_addr, size_t size, void** virtual_addr);
    void unmapMemoryRegion(void* virtual_addr, size_t size);
    
    // Register access helpers
    bool readRegisterArray(uint32_t base_offset, uint32_t* values, size_t count);
    bool writeRegisterArray(uint32_t base_offset, const uint32_t* values, size_t count);
    
    // DMA helpers
    bool allocateAlignedBuffer(size_t size, size_t alignment, void** virtual_addr, 
                              uint64_t* physical_addr);
    void freeAlignedBuffer(void* virtual_addr, size_t size);
    
    // Interrupt helpers
    bool setupInterruptThread();
    void interruptThreadFunction();
    void handleInterrupt(InterruptType type);
    
    // Device-specific helpers
    bool detectHardware();
    bool initializeHardware();
    bool setupInterrupts();
    bool setupDMA();
    void cleanupHardware();

private:
    // Interrupt handling
    std::vector<std::function<void(InterruptType)>> interrupt_handlers_;
    std::thread interrupt_thread_;
    std::atomic<bool> interrupt_running_;
    
    // Memory mapping
    struct MemoryMapping {
        void* virtual_addr;
        uint64_t physical_addr;
        size_t size;
    };
    std::vector<MemoryMapping> memory_mappings_;
    
    // Helper methods
    void cleanupMemoryMappings();
    bool findMemoryMapping(uint64_t physical_addr, size_t size, MemoryMapping& mapping);
};