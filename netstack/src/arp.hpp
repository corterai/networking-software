#pragma once
#include "raw_socket_driver.hpp"
#include "eth.hpp"
#include <optional>
#include <unordered_map>

enum class ArpOperation : uint16_t { Request = 1, Reply = 2 };

struct IPv4Address {
  uint32_t be{}; // network byte order
  static std::optional<IPv4Address> fromString(const std::string& s);
  std::string toString() const;
  bool operator==(const IPv4Address& o) const { return be == o.be; }
};

struct IPv4AddressHash {
  size_t operator()(const IPv4Address& a) const noexcept { return std::hash<uint32_t>{}(a.be); }
};

struct ArpPacket {
  ArpOperation operation{ArpOperation::Request};
  MacAddress sender_mac{};
  IPv4Address sender_ip{};
  MacAddress target_mac{};
  IPv4Address target_ip{};

  static std::optional<ArpPacket> parse(std::span<const std::byte> data);
  std::vector<std::byte> serialize() const;
  static ArpPacket makeReply(const MacAddress& own_mac, const IPv4Address& own_ip,
                             const MacAddress& target_mac, const IPv4Address& target_ip);
};

class ArpCache {
public:
  void update(const IPv4Address& ip, const MacAddress& mac) { cache_[ip] = mac; }
  std::optional<MacAddress> lookup(const IPv4Address& ip) const {
    auto it = cache_.find(ip);
    if (it == cache_.end()) return std::nullopt;
    return it->second;
  }
private:
  std::unordered_map<IPv4Address, MacAddress, IPv4AddressHash> cache_{};
};