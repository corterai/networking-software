#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct MacAddress {
  std::array<std::byte,6> bytes{};

  static std::optional<MacAddress> fromString(const std::string& s);
  std::string toString() const;
  bool operator==(const MacAddress& other) const { return bytes == other.bytes; }
  bool operator!=(const MacAddress& other) const { return !(*this == other); }
};

enum class EthType : uint16_t { IPv4 = 0x0800, ARP = 0x0806 };

struct EthFrame {
  MacAddress dst{};
  MacAddress src{};
  EthType ethertype{EthType::IPv4};
  std::vector<std::byte> payload{};
};

class RawSocketDriver {
public:
  using ReceiveHandler = std::function<void(const EthFrame&)>;

  bool open(const std::string& ifname, const std::optional<MacAddress>& override_mac = std::nullopt);
  void close();
  void setReceiveHandler(ReceiveHandler handler);
  void send(const EthFrame& frame);
  void pollOnce();
  MacAddress macAddress() const { return own_mac_; }

private:
  int fd_{-1};
  int ifindex_{-1};
  MacAddress own_mac_{};
  ReceiveHandler handler_{};
};