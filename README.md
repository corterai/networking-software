# Ethernet Networking Stack

A comprehensive C++ networking stack designed to run on commonly available Ethernet NIC controller chips. This project provides a modular, extensible architecture for implementing low-level network drivers and controllers.

## Overview

This networking stack is designed around the Intel 82599 (X540) 10 Gigabit Ethernet controller, but uses an abstract driver interface that allows easy porting to other NIC chips. The architecture separates hardware-specific driver logic from high-level network interface management.

## Architecture

### Core Components

#### 1. PacketBuffer (`include/packet_buffer.h`, `src/packet_buffer.cpp`)
- Efficient packet data management with zero-copy support
- Buffer chaining for large packets
- Metadata tracking (type, flags, timestamp, interface index)
- Reference counting for shared buffers
- Templated header access methods

#### 2. DriverInterface (`include/driver_interface.h`, `src/driver_interface.cpp`)
- Abstract base class for hardware-agnostic NIC interaction
- Memory management (DMA buffers, register access)
- Interrupt handling and device control
- Power management and statistics
- Common helper methods for driver implementations

#### 3. Intel82599Driver (`include/intel_82599_driver.h`)
- Concrete implementation for Intel 82599 (X540) controller
- Complete register definitions and constants
- Queue management (RX/TX rings)
- Advanced features: VLAN filtering, flow control, jumbo frames
- EEPROM operations and PHY management

#### 4. EthernetController (`include/ethernet_controller.h`, `src/ethernet_controller.cpp`)
- High-level interface to manage NIC hardware
- Configuration management and statistics
- Packet transmission and reception
- Link monitoring and status reporting
- Threading for RX, TX, and link monitoring

#### 5. NetworkInterface (`include/network_interface.h`)
- OS-level abstraction for network interfaces
- Address management (MAC, IP)
- Interface state and configuration
- Integration with EthernetController
- Callback-based event handling

## Features

- **Modular Design**: Clean separation between hardware drivers and network logic
- **Multi-threaded**: Separate threads for RX, TX, and link monitoring
- **Callback-based**: Event-driven architecture for packet reception and status changes
- **Zero-copy**: Efficient packet buffer management
- **Extensible**: Easy to add support for new NIC chips
- **Modern C++**: Uses C++17 features and RAII principles

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16 or higher
- Linux kernel headers (for hardware access)
- Root privileges (for direct hardware access)

## Building

### Prerequisites

```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config linux-headers-$(uname -r)
```

### Build Steps

```bash
# Clone the repository
git clone <repository-url>
cd ethernet-networking-stack

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Custom compiler
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

## Usage

### Basic Example

```cpp
#include "ethernet_controller.h"
#include "packet_buffer.h"

int main() {
    // Create controller
    auto controller = std::make_unique<EthernetController>("/dev/eth0");
    
    // Set callbacks
    controller->setPacketReceivedCallback([](std::unique_ptr<PacketBuffer> packet) {
        std::cout << "Received " << packet->size() << " bytes" << std::endl;
    });
    
    controller->setLinkStatusCallback([](bool link_up) {
        std::cout << "Link: " << (link_up ? "UP" : "DOWN") << std::endl;
    });
    
    // Initialize and configure
    if (!controller->initialize()) {
        std::cerr << "Initialization failed" << std::endl;
        return 1;
    }
    
    EthernetController::Configuration config;
    config.mtu_size = 1500;
    config.link_speed = 10000;  // 10 Gbps
    
    if (!controller->configure(config)) {
        std::cerr << "Configuration failed" << std::endl;
        return 1;
    }
    
    // Start controller
    if (!controller->start()) {
        std::cerr << "Start failed" << std::endl;
        return 1;
    }
    
    // Main loop
    while (true) {
        // Send test packet
        std::vector<uint8_t> packet = { /* packet data */ };
        controller->transmitPacket(packet.data(), packet.size());
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### Running the Example

```bash
# Build the example
make

# Run with root privileges (required for hardware access)
sudo ./networking_stack
```

## Configuration

### EthernetController Configuration

```cpp
EthernetController::Configuration config;
config.mac_address = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
config.mtu_size = 1500;
config.promiscuous_mode = false;
config.auto_negotiation = true;
config.link_speed = 10000;  // 10 Gbps
config.flow_control = true;
config.rx_ring_size = 1024;
config.tx_ring_size = 1024;
```

### Driver-Specific Configuration

The Intel 82599 driver supports additional configuration options:

```cpp
// Access driver-specific features
auto driver = dynamic_cast<Intel82599Driver*>(controller->getDriver());
if (driver) {
    driver->setVLANFiltering(true);
    driver->setJumboFrames(9000);
    driver->setFlowControl(Intel82599Driver::FlowControl::RX_TX);
}
```

## Extending the Stack

### Adding Support for New NIC Chips

1. Create a new driver class inheriting from `DriverInterface`
2. Implement all pure virtual methods
3. Add hardware-specific register definitions and constants
4. Update the factory method in `EthernetController`

Example:

```cpp
class NewNICDriver : public DriverInterface {
public:
    NewNICDriver(const std::string& device_name);
    
    // Implement required methods
    bool open() override;
    void close() override;
    bool isReady() const override;
    // ... other methods
};
```

### Custom Packet Processing

Extend the packet handling by creating custom packet processors:

```cpp
class CustomPacketProcessor {
public:
    virtual void processPacket(std::unique_ptr<PacketBuffer> packet) = 0;
    virtual ~CustomPacketProcessor() = default;
};

// Register with controller
controller->setPacketReceivedCallback(
    [&processor](std::unique_ptr<PacketBuffer> packet) {
        processor.processPacket(std::move(packet));
    }
);
```

## Performance Considerations

- **Ring Sizes**: Adjust RX/TX ring sizes based on your workload
- **Interrupt Moderation**: Configure interrupt coalescing for high-throughput scenarios
- **Buffer Alignment**: DMA buffers should be aligned to cache line boundaries
- **NUMA Awareness**: Consider NUMA node placement for multi-socket systems

## Debugging

### Enable Debug Output

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with debug output
sudo ./networking_stack
```

### Common Issues

1. **Permission Denied**: Ensure running with root privileges
2. **Device Not Found**: Check device path and driver loading
3. **DMA Errors**: Verify memory alignment and DMA capabilities
4. **Link Down**: Check cable connection and PHY status

## Testing

### Unit Tests

```bash
# Build tests
make test

# Run tests
./test_runner
```

### Integration Tests

```bash
# Test with real hardware
sudo ./test_integration

# Test with loopback
./test_loopback
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

### Code Style

- Follow the existing code style
- Use meaningful variable and function names
- Add comments for complex logic
- Include error handling for all operations

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Intel for the 82599 controller specifications
- Linux kernel community for driver reference implementations
- Contributors and maintainers

## Support

For questions and support:
- Create an issue on GitHub
- Check the documentation
- Review the example code
- Consult the troubleshooting guide

## Roadmap

- [ ] Support for additional NIC chips
- [ ] IPv6 support
- [ ] Advanced filtering and offloading
- [ ] Performance monitoring tools
- [ ] Container support
- [ ] Windows compatibility