#pragma once
#include "arp.hpp"
#include "eth.hpp"
#include <functional>
#include <optional>
#include <span>
#include <vector>

enum class IPv4ProtocolNumber : uint8_t { ICMP = 1, UDP = 17 };

enum class IPv4Accept { Accept, Drop };

class IPv4Protocol;

using IPv4TxCallback = std::function<void(const MacAddress&, std::span<const std::byte>)>;
using MacResolverCallback = std::function<std::optional<MacAddress>(const IPv4Address&)>;

class IPv4Dispatcher {
public:
  virtual ~IPv4Dispatcher() = default;
  virtual bool handle(const IPv4Address& src, const IPv4Address& dst,
                      uint8_t protocol, std::span<const std::byte> payload) = 0;
};

class IPv4Protocol {
public:
  explicit IPv4Protocol(IPv4Address own_ip) : own_ip_(own_ip) {}

  void setTransmitCallback(IPv4TxCallback cb) { tx_ = std::move(cb); }
  void setMacResolver(MacResolverCallback cb) { mac_resolver_ = std::move(cb); }
  std::optional<MacAddress> resolveMac(const IPv4Address& ip) const { return mac_resolver_ ? mac_resolver_(ip) : std::nullopt; }

  void receive(std::span<const std::byte> packet,
               std::function<IPv4Accept(const IPv4Address& dst)> filter,
               IPv4Dispatcher& icmp_disp,
               IPv4Dispatcher& udp_disp);

  void send(const IPv4Address& dst, uint8_t protocol, std::span<const std::byte> payload,
            const MacAddress& dst_mac);

  IPv4Address ownIp() const { return own_ip_; }

private:
  IPv4Address own_ip_{};
  IPv4TxCallback tx_{};
  MacResolverCallback mac_resolver_{};
};