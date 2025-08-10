#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Forward declarations
class PacketBuffer;
class NetworkInterface;
class DriverInterface;

/**
 * @brief Ethernet Controller class for Intel 82599 (X540) NIC
 * 
 * This class provides a high-level interface to the Ethernet controller
 * hardware, managing packet transmission, reception, and device configuration.
 */
class EthernetController {
public:
    // Controller states
    enum class State {
        UNINITIALIZED,
        INITIALIZING,
        READY,
        RUNNING,
        ERROR,
        STOPPED
    };

    // Packet types
    enum class PacketType {
        ETHERNET_II,
        VLAN_TAGGED,
        JUMBO_FRAME
    };

    // Statistics structure
    struct Statistics {
        uint64_t packets_received;
        uint64_t packets_transmitted;
        uint64_t bytes_received;
        uint64_t bytes_transmitted;
        uint64_t rx_errors;
        uint64_t tx_errors;
        uint64_t rx_dropped;
        uint64_t tx_dropped;
    };

    // Configuration structure
    struct Configuration {
        uint8_t mac_address[6];
        uint32_t mtu_size;
        bool promiscuous_mode;
        bool auto_negotiation;
        uint32_t link_speed;
        bool flow_control;
        uint32_t rx_ring_size;
        uint32_t tx_ring_size;
    };

    // Callback types
    using PacketReceivedCallback = std::function<void(std::unique_ptr<PacketBuffer>)>;
    using LinkStatusCallback = std::function<void(bool link_up)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    explicit EthernetController(const std::string& device_name);
    ~EthernetController();

    // Initialization and control
    bool initialize();
    bool start();
    void stop();
    void reset();

    // Configuration
    bool configure(const Configuration& config);
    Configuration getConfiguration() const;
    bool setMACAddress(const uint8_t* mac_address);
    bool setMTU(uint32_t mtu);
    bool setPromiscuousMode(bool enabled);

    // Packet operations
    bool transmitPacket(std::unique_ptr<PacketBuffer> packet);
    bool transmitPacket(const uint8_t* data, size_t length);
    
    // Statistics and status
    Statistics getStatistics() const;
    State getState() const;
    bool isLinkUp() const;
    uint32_t getLinkSpeed() const;

    // Callback registration
    void setPacketReceivedCallback(PacketReceivedCallback callback);
    void setLinkStatusCallback(LinkStatusCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // Hardware-specific operations
    bool readRegister(uint32_t offset, uint32_t& value);
    bool writeRegister(uint32_t offset, uint32_t value);
    bool readEEPROM(uint16_t offset, uint16_t& value);
    bool writeEEPROM(uint16_t offset, uint16_t value);

private:
    // Hardware abstraction
    std::unique_ptr<DriverInterface> driver_;
    
    // Configuration and state
    Configuration config_;
    State state_;
    std::atomic<bool> link_up_;
    std::atomic<uint32_t> link_speed_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // Callbacks
    PacketReceivedCallback packet_received_callback_;
    LinkStatusCallback link_status_callback_;
    ErrorCallback error_callback_;
    
    // Threading
    std::thread rx_thread_;
    std::thread tx_thread_;
    std::thread link_monitor_thread_;
    std::atomic<bool> running_;
    
    // Synchronization
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Internal methods
    void rxThreadFunction();
    void txThreadFunction();
    void linkMonitorThreadFunction();
    void updateStatistics();
    void handleError(const std::string& error);
    bool setupHardware();
    bool setupDMA();
    bool setupInterrupts();
    void cleanup();
    
    // Hardware-specific methods
    bool resetHardware();
    bool initializeHardware();
    bool setupReceiveRings();
    bool setupTransmitRings();
    bool enableInterrupts();
    bool disableInterrupts();
    void processInterrupt();
    
    // Packet processing
    void processReceivedPacket(std::unique_ptr<PacketBuffer> packet);
    bool validatePacket(const PacketBuffer& packet);
    void updatePacketStatistics(const PacketBuffer& packet, bool received);
};