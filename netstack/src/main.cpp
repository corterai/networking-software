#include "raw_socket_driver.hpp"
#include "eth.hpp"
#include "arp.hpp"
#include "ipv4.hpp"
#include "icmp.hpp"
#include "udp.hpp"

#include <csignal>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>

static volatile std::sig_atomic_t g_running = 1;

static void handle_signal(int) { g_running = 0; }

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <iface> <ipv4> [mac]" << std::endl;
    return 1;
  }
  const std::string iface = argv[1];
  const std::string ipv4_str = argv[2];
  std::optional<MacAddress> own_mac;
  if (argc >= 4) {
    auto mac = MacAddress::fromString(argv[3]);
    if (!mac.has_value()) {
      std::cerr << "Invalid MAC address format" << std::endl;
      return 1;
    }
    own_mac = mac;
  }

  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  RawSocketDriver driver;
  if (!driver.open(iface, own_mac)) {
    std::cerr << "Failed to open raw socket on interface: " << iface << std::endl;
    return 1;
  }

  const auto own_ipv4 = IPv4Address::fromString(ipv4_str);
  if (!own_ipv4.has_value()) {
    std::cerr << "Invalid IPv4 address" << std::endl;
    return 1;
  }

  ArpCache arp_cache;
  IPv4Protocol ipv4_proto(*own_ipv4);
  ipv4_proto.setMacResolver([&](const IPv4Address& ip){ return arp_cache.lookup(ip); });
  ICMPProtocol icmp_proto(ipv4_proto);
  UDPProtocol udp_proto(ipv4_proto);

  ipv4_proto.setTransmitCallback([&](const MacAddress& dst_mac, std::span<const std::byte> payload){
    EthFrame frame;
    frame.dst = dst_mac;
    frame.src = driver.macAddress();
    frame.ethertype = EthType::IPv4;
    frame.payload.assign(payload.begin(), payload.end());
    driver.send(frame);
  });

  udp_proto.setReceiveHandler([](const IPv4Address& src, uint16_t src_port, uint16_t dst_port, std::span<const std::byte> data){
    std::cout << "UDP packet from " << src.toString() << ":" << src_port
              << " to port " << dst_port << ", length=" << data.size() << std::endl;
    return true; // consumed
  });

  icmp_proto.setEchoReplyEnabled(true);

  driver.setReceiveHandler([&](const EthFrame& frame){
    switch (frame.ethertype) {
      case EthType::ARP: {
        auto maybe_arp = ArpPacket::parse(frame.payload);
        if (!maybe_arp.has_value()) return;
        auto& pkt = *maybe_arp;
        if (pkt.operation == ArpOperation::Request && pkt.target_ip == *own_ipv4) {
          ArpPacket reply = ArpPacket::makeReply(driver.macAddress(), *own_ipv4, pkt.sender_mac, pkt.sender_ip);
          EthFrame out;
          out.dst = pkt.sender_mac;
          out.src = driver.macAddress();
          out.ethertype = EthType::ARP;
          out.payload = reply.serialize();
          driver.send(out);
        }
        if (pkt.operation == ArpOperation::Reply || pkt.operation == ArpOperation::Request) {
          arp_cache.update(pkt.sender_ip, pkt.sender_mac);
          std::cout << "ARP: " << pkt.sender_ip.toString() << " is at " << pkt.sender_mac.toString() << std::endl;
        }
        break;
      }
      case EthType::IPv4: {
        ipv4_proto.receive(frame.payload, [&](const IPv4Address& dst){
          if (dst == *own_ipv4) return IPv4Accept::Accept;
          return IPv4Accept::Drop;
        }, icmp_proto.dispatcher(), udp_proto.dispatcher());
        break;
      }
      default: break;
    }
  });

  std::cout << "netstack running on " << iface << " IPv4=" << own_ipv4->toString()
            << " MAC=" << driver.macAddress().toString() << std::endl;

  while (g_running) {
    driver.pollOnce();
  }

  driver.close();
  return 0;
}