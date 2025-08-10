#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include "ethernet_controller.h"
#include "network_interface.h"
#include "packet_buffer.h"

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

void packetReceivedCallback(std::unique_ptr<PacketBuffer> packet) {
    std::cout << "Received packet: " << packet->size() << " bytes" << std::endl;
    
    // Print first few bytes of the packet
    if (packet->size() > 0) {
        std::cout << "First 16 bytes: ";
        for (size_t i = 0; i < std::min(packet->size(), size_t(16)); ++i) {
            printf("%02X ", packet->data()[i]);
        }
        std::cout << std::endl;
    }
}

void linkStatusCallback(bool link_up) {
    std::cout << "Link status changed: " << (link_up ? "UP" : "DOWN") << std::endl;
}

void errorCallback(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Ethernet Networking Stack Example" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Create Ethernet controller
        std::cout << "Creating Ethernet controller..." << std::endl;
        auto controller = std::make_unique<EthernetController>("/dev/eth0");
        
        // Set callbacks
        controller->setPacketReceivedCallback(packetReceivedCallback);
        controller->setLinkStatusCallback(linkStatusCallback);
        controller->setErrorCallback(errorCallback);
        
        // Initialize controller
        std::cout << "Initializing controller..." << std::endl;
        if (!controller->initialize()) {
            std::cerr << "Failed to initialize controller" << std::endl;
            return 1;
        }
        
        // Configure controller
        std::cout << "Configuring controller..." << std::endl;
        EthernetController::Configuration config;
        std::memset(config.mac_address, 0, sizeof(config.mac_address));
        config.mac_address[0] = 0x00;
        config.mac_address[1] = 0x11;
        config.mac_address[2] = 0x22;
        config.mac_address[3] = 0x33;
        config.mac_address[4] = 0x44;
        config.mac_address[5] = 0x55;
        config.mtu_size = 1500;
        config.promiscuous_mode = false;
        config.auto_negotiation = true;
        config.link_speed = 10000;  // 10 Gbps
        config.flow_control = true;
        config.rx_ring_size = 1024;
        config.tx_ring_size = 1024;
        
        if (!controller->configure(config)) {
            std::cerr << "Failed to configure controller" << std::endl;
            return 1;
        }
        
        // Start controller
        std::cout << "Starting controller..." << std::endl;
        if (!controller->start()) {
            std::cerr << "Failed to start controller" << std::endl;
            return 1;
        }
        
        std::cout << "Controller started successfully" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        // Main loop
        while (running) {
            // Print status every 5 seconds
            static auto last_status = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - last_status >= std::chrono::seconds(5)) {
                auto state = controller->getState();
                auto link_up = controller->isLinkUp();
                auto link_speed = controller->getLinkSpeed();
                auto stats = controller->getStatistics();
                
                std::cout << "\n--- Status ---" << std::endl;
                std::cout << "State: ";
                switch (state) {
                    case EthernetController::State::UNINITIALIZED: std::cout << "UNINITIALIZED"; break;
                    case EthernetController::State::INITIALIZING: std::cout << "INITIALIZING"; break;
                    case EthernetController::State::READY: std::cout << "READY"; break;
                    case EthernetController::State::RUNNING: std::cout << "RUNNING"; break;
                    case EthernetController::State::ERROR: std::cout << "ERROR"; break;
                    case EthernetController::State::STOPPED: std::cout << "STOPPED"; break;
                }
                std::cout << std::endl;
                
                std::cout << "Link: " << (link_up ? "UP" : "DOWN") << std::endl;
                if (link_up) {
                    std::cout << "Speed: " << link_speed << " Mbps" << std::endl;
                }
                
                std::cout << "Statistics:" << std::endl;
                std::cout << "  RX packets: " << stats.packets_received << std::endl;
                std::cout << "  TX packets: " << stats.packets_transmitted << std::endl;
                std::cout << "  RX bytes: " << stats.bytes_received << std::endl;
                std::cout << "  TX bytes: " << stats.bytes_transmitted << std::endl;
                std::cout << "  RX errors: " << stats.rx_errors << std::endl;
                std::cout << "  TX errors: " << stats.tx_errors << std::endl;
                
                last_status = now;
            }
            
            // Send a test packet every 10 seconds
            static auto last_packet = std::chrono::steady_clock::now();
            if (now - last_packet >= std::chrono::seconds(10)) {
                // Create a simple test packet (Ethernet frame)
                std::vector<uint8_t> test_packet = {
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination MAC (broadcast)
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  // Source MAC
                    0x08, 0x00,                            // EtherType (IPv4)
                    0x45, 0x00, 0x00, 0x14,               // IP header
                    0x00, 0x00, 0x40, 0x00,               // IP header
                    0x40, 0x11, 0x00, 0x00,               // IP header + UDP
                    0x7F, 0x00, 0x00, 0x01,               // Source IP
                    0x7F, 0x00, 0x00, 0x01,               // Destination IP
                    0x00, 0x00, 0x00, 0x00,               // Source port, Destination port
                    0x00, 0x00, 0x00, 0x00                // Length, Checksum
                };
                
                if (controller->transmitPacket(test_packet.data(), test_packet.size())) {
                    std::cout << "Sent test packet: " << test_packet.size() << " bytes" << std::endl;
                } else {
                    std::cout << "Failed to send test packet" << std::endl;
                }
                
                last_packet = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Stop controller
        std::cout << "Stopping controller..." << std::endl;
        controller->stop();
        
        std::cout << "Controller stopped successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}