#pragma once
#include "ipv4.hpp"
#include <functional>

class UDPProtocol : public IPv4Dispatcher {
public:
  using ReceiveHandler = std::function<bool(const IPv4Address&, uint16_t, uint16_t, std::span<const std::byte>)>;

  explicit UDPProtocol(IPv4Protocol& ipv4) : ipv4_(ipv4) {}

  void setReceiveHandler(ReceiveHandler h) { handler_ = std::move(h); }

  bool handle(const IPv4Address& src, const IPv4Address& dst,
              uint8_t protocol, std::span<const std::byte> payload) override;

  IPv4Dispatcher& dispatcher() { return *this; }

private:
  IPv4Protocol& ipv4_;
  ReceiveHandler handler_{};
};