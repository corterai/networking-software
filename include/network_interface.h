#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>

// Forward declarations
class EthernetController;
class PacketBuffer;

/**
 * @brief Network interface class for OS-level networking
 * 
 * This class provides a network interface abstraction that integrates
 * with the operating system's networking stack.
 */
class NetworkInterface {
public:
    // Interface types
    enum class Type {
        ETHERNET,
        WIFI,
        LOOPBACK,
        TUNNEL,
        BRIDGE
    };

    // Interface states
    enum class State {
        DOWN,
        UP,
        RUNNING,
        ERROR,
        DISABLED
    };

    // Interface flags
    enum class Flags : uint32_t {
        NONE = 0x00000000,
        UP = 0x00000001,
        RUNNING = 0x00000002,
        MULTICAST = 0x00000004,
        BROADCAST = 0x00000008,
        POINTTOPOINT = 0x00000010,
        PROMISC = 0x00000020,
        ALLMULTI = 0x00000040,
        NOARP = 0x00000080,
        DYNAMIC = 0x00000100,
        SLAVE = 0x00000200,
        MASTER = 0x00000400
    };

    // Address family
    enum class AddressFamily {
        UNSPEC,
        INET,      // IPv4
        INET6,     // IPv6
        PACKET,    // Low-level packet interface
        LINK       // Link-level interface
    };

    // Address structure
    struct Address {
        AddressFamily family;
        std::vector<uint8_t> data;
        uint32_t prefix_length;
        
        Address() : family(AddressFamily::UNSPEC), prefix_length(0) {}
        Address(AddressFamily f, const std::vector<uint8_t>& d, uint32_t p = 0)
            : family(f), data(d), prefix_length(p) {}
    };

    // Interface statistics
    struct InterfaceStats {
        uint64_t rx_packets;
        uint64_t tx_packets;
        uint64_t rx_bytes;
        uint64_t tx_bytes;
        uint64_t rx_errors;
        uint64_t tx_errors;
        uint64_t rx_dropped;
        uint64_t tx_dropped;
        uint64_t rx_fifo_errors;
        uint64_t tx_fifo_errors;
        uint64_t rx_frame_errors;
        uint64_t tx_carrier_errors;
        uint64_t rx_compressed;
        uint64_t tx_compressed;
        uint64_t multicast;
    };

    // Constructor
    explicit NetworkInterface(const std::string& name);
    
    // Destructor
    ~NetworkInterface();

    // Interface identification
    std::string getName() const;
    std::string getDescription() const;
    Type getType() const;
    uint32_t getIndex() const;
    
    // State management
    State getState() const;
    bool isUp() const;
    bool isRunning() const;
    bool setState(State state);
    
    // Flags management
    Flags getFlags() const;
    void setFlags(Flags flags);
    void addFlags(Flags flags);
    void removeFlags(Flags flags);
    bool hasFlags(Flags flags) const;
    
    // Address management
    bool addAddress(const Address& address);
    bool removeAddress(const Address& address);
    std::vector<Address> getAddresses() const;
    Address getPrimaryAddress(AddressFamily family) const;
    
    // MAC address
    bool setMACAddress(const uint8_t* mac_address);
    bool getMACAddress(uint8_t* mac_address) const;
    
    // MTU configuration
    uint32_t getMTU() const;
    bool setMTU(uint32_t mtu);
    
    // Link configuration
    bool setLinkSpeed(uint32_t speed);
    uint32_t getLinkSpeed() const;
    bool setDuplex(bool full_duplex);
    bool isFullDuplex() const;
    
    // Statistics
    InterfaceStats getStatistics() const;
    void resetStatistics();
    void updateStatistics(const PacketBuffer& packet, bool received);
    
    // Packet handling
    bool sendPacket(std::unique_ptr<PacketBuffer> packet);
    bool sendPacket(const uint8_t* data, size_t length);
    void receivePacket(std::unique_ptr<PacketBuffer> packet);
    
    // Callback registration
    using PacketReceivedCallback = std::function<void(std::unique_ptr<PacketBuffer>)>;
    using PacketTransmittedCallback = std::function<void(std::unique_ptr<PacketBuffer>)>;
    using StateChangeCallback = std::function<void(State old_state, State new_state)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setPacketReceivedCallback(PacketReceivedCallback callback);
    void setPacketTransmittedCallback(PacketTransmittedCallback callback);
    void setStateChangeCallback(StateChangeCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Hardware interface
    void setHardwareController(std::shared_ptr<EthernetController> controller);
    std::shared_ptr<EthernetController> getHardwareController() const;
    
    // Configuration
    bool configure(const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> getConfiguration() const;
    
    // Monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // Utility methods
    std::string toString() const;
    bool isValid() const;
    void validate();

private:
    // Basic properties
    std::string name_;
    std::string description_;
    Type type_;
    uint32_t index_;
    
    // State and flags
    State state_;
    Flags flags_;
    
    // Addresses
    mutable std::mutex addresses_mutex_;
    std::vector<Address> addresses_;
    
    // MAC address
    uint8_t mac_address_[6];
    
    // Configuration
    uint32_t mtu_;
    uint32_t link_speed_;
    bool full_duplex_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    InterfaceStats stats_;
    
    // Callbacks
    PacketReceivedCallback packet_received_callback_;
    PacketTransmittedCallback packet_transmitted_callback_;
    StateChangeCallback state_change_callback_;
    ErrorCallback error_callback_;
    
    // Hardware controller
    std::shared_ptr<EthernetController> hardware_controller_;
    
    // Monitoring
    std::thread monitor_thread_;
    std::atomic<bool> monitoring_;
    std::atomic<bool> running_;
    
    // Internal methods
    void monitorThreadFunction();
    void updateState(State new_state);
    void handleError(const std::string& error);
    bool validateAddress(const Address& address) const;
    void notifyStateChange(State old_state, State new_state);
    void notifyPacketReceived(std::unique_ptr<PacketBuffer> packet);
    void notifyPacketTransmitted(std::unique_ptr<PacketBuffer> packet);
    
    // Hardware integration
    void setupHardwareCallbacks();
    void removeHardwareCallbacks();
    bool forwardPacketToHardware(std::unique_ptr<PacketBuffer> packet);
    
    // Configuration validation
    bool validateConfiguration(const std::map<std::string, std::string>& config);
    bool applyConfiguration(const std::map<std::string, std::string>& config);
    
    // Statistics helpers
    void incrementRxStats(const PacketBuffer& packet);
    void incrementTxStats(const PacketBuffer& packet);
    void incrementErrorStats(bool rx_error);
};

// Flag operations
inline NetworkInterface::Flags operator|(NetworkInterface::Flags a, NetworkInterface::Flags b) {
    return static_cast<NetworkInterface::Flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline NetworkInterface::Flags operator&(NetworkInterface::Flags a, NetworkInterface::Flags b) {
    return static_cast<NetworkInterface::Flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline NetworkInterface::Flags operator~(NetworkInterface::Flags a) {
    return static_cast<NetworkInterface::Flags>(~static_cast<uint32_t>(a));
}

inline NetworkInterface::Flags& operator|=(NetworkInterface::Flags& a, NetworkInterface::Flags b) {
    a = a | b;
    return a;
}

inline NetworkInterface::Flags& operator&=(NetworkInterface::Flags& a, NetworkInterface::Flags b) {
    a = a & b;
    return a;
}