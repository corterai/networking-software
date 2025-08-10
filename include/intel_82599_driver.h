#pragma once

#include "driver_interface.h"
#include <cstdint>
#include <vector>
#include <memory>

/**
 * @brief Intel 82599 (X540) Ethernet Controller Driver
 * 
 * This class provides a concrete implementation of the DriverInterface
 * specifically for the Intel 82599/X540 Ethernet controller.
 */
class Intel82599Driver : public DriverInterface {
public:
    // Intel 82599 specific register offsets
    enum Registers {
        // Control registers
        CTRL = 0x00000,
        CTRL_EXT = 0x00018,
        STATUS = 0x00008,
        
        // MAC registers
        EEMNGCTL = 0x01010,
        EEMNGDATA = 0x01014,
        EERD = 0x01014,
        EEWR = 0x01018,
        
        // Receive registers
        RCTL = 0x01000,
        RDBAL = 0x02800,
        RDBAH = 0x02804,
        RDLEN = 0x02808,
        RDH = 0x02810,
        RDT = 0x02818,
        RDTR = 0x02820,
        RXDCTL = 0x02828,
        
        // Transmit registers
        TCTL = 0x00400,
        TDBAL = 0x03800,
        TDBAH = 0x03804,
        TDLEN = 0x03808,
        TDH = 0x03810,
        TDT = 0x03818,
        TXDCTL = 0x03828,
        
        // Interrupt registers
        IMS = 0x000D0,
        IMC = 0x000D8,
        ICS = 0x000C8,
        ICR = 0x000C0,
        
        // Flow control
        FCTRL = 0x00500,
        FCRTH = 0x00508,
        FCTTV = 0x00510,
        
        // Statistics registers
        GPRC = 0x04074,
        GPTC = 0x04078,
        GORC = 0x04080,
        GOTC = 0x04084,
        RNBC = 0x04088,
        RUC = 0x0408C,
        RFC = 0x04090,
        ROC = 0x04094,
        RJC = 0x04098,
        TORL = 0x040A0,
        TORH = 0x040A4,
        TOTL = 0x040A8,
        TOTH = 0x040AC,
        TPR = 0x040B0,
        TPT = 0x040B4,
        PTC64 = 0x040B8,
        PTC127 = 0x040BC,
        PTC255 = 0x040C0,
        PTC511 = 0x040C4,
        PTC1023 = 0x040C8,
        PTC1522 = 0x040CC,
        
        // Link registers
        LINKS = 0x042A4,
        AUTOC = 0x042A0,
        AUTOC2 = 0x042A8,
        AUTOC_LMS = 0x042A0,
        AUTOC_LMS_MASK = 0x00000003,
        AUTOC_LMS_1G_S = 0x00000000,
        AUTOC_LMS_10G_S = 0x00000001,
        AUTOC_LMS_10G_S_GMII = 0x00000002,
        AUTOC_LMS_10G_S_XAUI = 0x00000003,
        
        // EEPROM registers
        EERD_START = 0x00000000,
        EERD_DONE = 0x00000010,
        EERD_DATA = 0x0000FFFF,
        EERD_ADDR = 0x000FF000,
        EERD_ADDR_SHIFT = 12,
        
        // Receive descriptor control
        RCTL_EN = 0x00000002,
        RCTL_SBP = 0x00000004,
        RCTL_UPE = 0x00000008,
        RCTL_MPE = 0x00000010,
        RCTL_LPE = 0x00000020,
        RCTL_LBM = 0x000000C0,
        RCTL_LBM_NONE = 0x00000000,
        RCTL_LBM_MAC = 0x00000040,
        RCTL_LBM_PHY = 0x00000080,
        RCTL_RDMTS = 0x00000300,
        RCTL_RDMTS_1_2 = 0x00000000,
        RCTL_RDMTS_1_4 = 0x00000100,
        RCTL_RDMTS_1_8 = 0x00000200,
        RCTL_MO = 0x00000C00,
        RCTL_MO_36 = 0x00000000,
        RCTL_MO_35 = 0x00000400,
        RCTL_MO_34 = 0x00000800,
        RCTL_MO_32 = 0x00000C00,
        RCTL_BAM = 0x00008000,
        RCTL_BSIZE = 0x00030000,
        RCTL_BSIZE_2048 = 0x00000000,
        RCTL_BSIZE_1024 = 0x00010000,
        RCTL_BSIZE_512 = 0x00020000,
        RCTL_BSIZE_256 = 0x00030000,
        RCTL_VFE = 0x00040000,
        RCTL_CFIEN = 0x00080000,
        RCTL_CFI = 0x00100000,
        RCTL_DPF = 0x00400000,
        RCTL_PMCF = 0x00800000,
        RCTL_SECRC = 0x04000000,
        
        // Transmit descriptor control
        TCTL_EN = 0x00000002,
        TCTL_PSP = 0x00000008,
        TCTL_CT = 0x00000FF0,
        TCTL_COLD = 0x003FF000,
        TCTL_COLD_SHIFT = 12,
        TCTL_SWXOFF = 0x00400000,
        TCTL_RTLC = 0x01000000,
        TCTL_NRTU = 0x02000000,
        
        // Interrupt control
        IMS_LSC = 0x00000004,
        IMS_RXT0 = 0x00000080,
        IMS_RXT1 = 0x00000100,
        IMS_RXT2 = 0x00000200,
        IMS_RXT3 = 0x00000400,
        IMS_RXT4 = 0x00000800,
        IMS_RXT5 = 0x00001000,
        IMS_RXT6 = 0x00002000,
        IMS_RXT7 = 0x00004000,
        IMS_TXQE = 0x00000002,
        IMS_TXDW = 0x00000001,
        IMS_DRSTA = 0x00000008,
        IMS_RXDMT0 = 0x00000010,
        IMS_RXDMT1 = 0x00000020,
        IMS_RXDMT2 = 0x00000040,
        IMS_RXDMT3 = 0x00000080,
        IMS_RXDMT4 = 0x00000100,
        IMS_RXDMT5 = 0x00000200,
        IMS_RXDMT6 = 0x00000400,
        IMS_RXDMT7 = 0x00000800,
        IMS_TXQE = 0x00000002,
        IMS_TXDW = 0x00000001,
        IMS_DRSTA = 0x00000008,
        IMS_RXDMT0 = 0x00000010,
        IMS_RXDMT1 = 0x00000020,
        IMS_RXDMT2 = 0x00000040,
        IMS_RXDMT3 = 0x00000080,
        IMS_RXDMT4 = 0x00000100,
        IMS_RXDMT5 = 0x00000200,
        IMS_RXDMT6 = 0x00000400,
        IMS_RXDMT7 = 0x00000800
    };

    // Intel 82599 specific constants
    enum Constants {
        MAX_RX_QUEUES = 128,
        MAX_TX_QUEUES = 128,
        MAX_RX_DESC = 4096,
        MAX_TX_DESC = 4096,
        DEFAULT_RX_DESC = 1024,
        DEFAULT_TX_DESC = 1024,
        MAX_FRAME_SIZE = 16384,
        MIN_FRAME_SIZE = 64,
        DEFAULT_MTU = 1500,
        EEPROM_SIZE = 64 * 1024,  // 64KB
        EEPROM_WORD_SIZE = 2,
        EEPROM_READ_TIMEOUT = 1000,
        EEPROM_WRITE_TIMEOUT = 1000,
        PHY_RESET_TIMEOUT = 1000,
        LINK_RESET_TIMEOUT = 1000,
        AUTONEG_TIMEOUT = 10000,
        FLOW_CONTROL_PAUSE_TIME = 0xFFFF,
        FLOW_CONTROL_HIGH_WATER = 0x8000,
        FLOW_CONTROL_LOW_WATER = 0x4000
    };

    // Intel 82599 specific descriptor structures
    struct RxDescriptor {
        uint64_t buffer_address;
        uint16_t length;
        uint16_t checksum;
        uint8_t status;
        uint8_t errors;
        uint16_t special;
    } __attribute__((packed));

    struct TxDescriptor {
        uint64_t buffer_address;
        uint16_t length;
        uint8_t cso;
        uint8_t cmd;
        uint8_t status;
        uint8_t css;
        uint16_t special;
    } __attribute__((packed));

    // Constructor
    explicit Intel82599Driver(const std::string& device_path);
    
    // Destructor
    ~Intel82599Driver() override;

    // Device management
    bool open() override;
    void close() override;
    bool isReady() const override;
    bool reset() override;

    // Memory access
    bool readMemory(MemoryType type, uint64_t offset, void* buffer, size_t size) override;
    bool writeMemory(MemoryType type, uint64_t offset, const void* buffer, size_t size) override;
    
    // Register access
    bool readRegister(uint32_t offset, uint32_t& value) override;
    bool writeRegister(uint32_t offset, uint32_t value) override;
    bool readRegisterMasked(uint32_t offset, uint32_t& value, uint32_t mask) override;
    bool writeRegisterMasked(uint32_t offset, uint32_t value, uint32_t mask) override;

    // DMA operations
    bool allocateDMABuffer(size_t size, void** virtual_addr, uint64_t* physical_addr) override;
    void freeDMABuffer(void* virtual_addr, size_t size) override;
    bool mapDMABuffer(void* virtual_addr, size_t size, uint64_t* physical_addr) override;
    void unmapDMABuffer(void* virtual_addr, size_t size) override;
    
    // DMA descriptor management
    bool setupDMADescriptors(std::vector<DMADescriptor>& descriptors, 
                           const std::vector<void*>& buffers, 
                           const std::vector<size_t>& sizes) override;
    bool updateDMADescriptor(uint32_t index, const DMADescriptor& descriptor) override;
    bool getDMADescriptorStatus(uint32_t index, uint32_t& status) override;

    // Interrupt handling
    bool enableInterrupts() override;
    bool disableInterrupts() override;
    bool registerInterruptHandler(InterruptType type, 
                                std::function<void(InterruptType)> handler) override;
    bool unregisterInterruptHandler(InterruptType type) override;
    bool waitForInterrupt(InterruptType type, int timeout_ms = -1) override;
    bool acknowledgeInterrupt(InterruptType type) override;

    // Device-specific operations
    bool getDeviceInfo(std::string& vendor, std::string& device, 
                     std::string& revision) override;
    bool getMACAddress(uint8_t* mac_address) override;
    bool setMACAddress(const uint8_t* mac_address) override;
    bool getLinkStatus(bool& link_up, uint32_t& speed, bool& duplex) override;
    bool setLinkSpeed(uint32_t speed, bool duplex) override;

    // Power management
    bool enterLowPowerMode() override;
    bool exitLowPowerMode() override;
    bool isLowPowerMode() const override;

    // Statistics
    bool getHardwareStatistics(uint64_t* stats_array, size_t count) override;
    bool resetHardwareStatistics() override;

    // Error handling
    std::string getLastError() const override;
    void clearLastError() override;
    bool hasError() const override;

    // Intel 82599 specific methods
    bool initializeController();
    bool setupReceiveQueues(uint32_t num_queues, uint32_t desc_per_queue);
    bool setupTransmitQueues(uint32_t num_queues, uint32_t desc_per_queue);
    bool enableReceiveQueue(uint32_t queue_id);
    bool enableTransmitQueue(uint32_t queue_id);
    bool disableReceiveQueue(uint32_t queue_id);
    bool disableTransmitQueue(uint32_t queue_id);
    bool setReceiveQueueRateLimit(uint32_t queue_id, uint32_t rate);
    bool setTransmitQueueRateLimit(uint32_t queue_id, uint32_t rate);
    bool setFlowControl(bool enable, uint32_t high_water, uint32_t low_water);
    bool setJumboFrames(bool enable, uint32_t max_size);
    bool setVLANFiltering(bool enable);
    bool addVLANFilter(uint16_t vlan_id);
    bool removeVLANFilter(uint16_t vlan_id);
    bool setPromiscuousMode(bool enable);
    bool setMulticastPromiscuous(bool enable);
    bool addMulticastAddress(const uint8_t* address);
    bool removeMulticastAddress(const uint8_t* address);
    bool setInterruptModeration(uint32_t interval);
    bool setReceiveBufferSize(uint32_t size);
    bool setTransmitBufferSize(uint32_t size);
    bool setAutoNegotiation(bool enable);
    bool setLinkSpeed(uint32_t speed, bool duplex, bool auto_neg);
    bool resetPHY();
    bool resetLink();
    bool waitForLink(uint32_t timeout_ms);
    bool waitForAutoNegotiation(uint32_t timeout_ms);
    bool readEEPROM(uint16_t offset, uint16_t& value);
    bool writeEEPROM(uint16_t offset, uint16_t value);
    bool validateEEPROM();
    bool getEEPROMChecksum(uint16_t& checksum);
    bool setEEPROMChecksum(uint16_t checksum);

private:
    // Hardware state
    bool controller_initialized_;
    bool receive_enabled_;
    bool transmit_enabled_;
    bool interrupts_enabled_;
    bool flow_control_enabled_;
    bool jumbo_frames_enabled_;
    bool vlan_filtering_enabled_;
    bool promiscuous_mode_;
    bool multicast_promiscuous_;
    
    // Queue configuration
    uint32_t num_rx_queues_;
    uint32_t num_tx_queues_;
    uint32_t rx_desc_per_queue_;
    uint32_t tx_desc_per_queue_;
    
    // Hardware resources
    std::vector<void*> rx_buffers_;
    std::vector<void*> tx_buffers_;
    std::vector<uint64_t> rx_buffer_phys_;
    std::vector<uint64_t> tx_buffer_phys_;
    std::vector<RxDescriptor*> rx_descriptors_;
    std::vector<TxDescriptor*> tx_descriptors_;
    std::vector<uint64_t> rx_descriptor_phys_;
    std::vector<uint64_t> tx_descriptor_phys_;
    
    // Interrupt handling
    std::vector<std::function<void(InterruptType)>> interrupt_handlers_;
    std::thread interrupt_thread_;
    std::atomic<bool> interrupt_running_;
    
    // Internal methods
    bool detectHardware() override;
    bool initializeHardware() override;
    bool setupInterrupts() override;
    bool setupDMA() override;
    void cleanupHardware() override;
    
    // Hardware-specific initialization
    bool resetController();
    bool initializeMAC();
    bool initializePHY();
    bool setupReceiveDescriptors();
    bool setupTransmitDescriptors();
    bool setupInterruptModeration();
    bool setupFlowControl();
    bool setupVLANFiltering();
    bool setupMulticastFiltering();
    
    // Queue management
    bool allocateReceiveBuffers();
    bool allocateTransmitBuffers();
    bool freeReceiveBuffers();
    bool freeTransmitBuffers();
    bool setupReceiveQueue(uint32_t queue_id);
    bool setupTransmitQueue(uint32_t queue_id);
    bool cleanupReceiveQueue(uint32_t queue_id);
    bool cleanupTransmitQueue(uint32_t queue_id);
    
    // Interrupt handling
    void interruptThreadFunction();
    void handleHardwareInterrupt();
    bool setupMSIInterrupts();
    bool setupMSIXInterrupts();
    bool setupLegacyInterrupts();
    
    // EEPROM operations
    bool waitForEEPROMReady(uint32_t timeout_ms);
    bool waitForEEPROMWriteComplete(uint32_t timeout_ms);
    bool calculateEEPROMChecksum(uint16_t& checksum);
    
    // PHY operations
    bool waitForPHYReset(uint32_t timeout_ms);
    bool waitForLinkReset(uint32_t timeout_ms);
    bool waitForAutoNegotiationComplete(uint32_t timeout_ms);
    
    // Utility methods
    bool isValidQueueId(uint32_t queue_id) const;
    bool isValidDescriptorCount(uint32_t count) const;
    bool isValidBufferSize(uint32_t size) const;
    bool isValidLinkSpeed(uint32_t speed) const;
    uint32_t calculateInterruptModeration(uint32_t interval);
    uint32_t calculateFlowControlThresholds(uint32_t high_water, uint32_t low_water);
};