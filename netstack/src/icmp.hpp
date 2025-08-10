#pragma once
#include "ipv4.hpp"

class ICMPProtocol : public IPv4Dispatcher {
public:
  explicit ICMPProtocol(IPv4Protocol& ipv4) : ipv4_(ipv4) {}

  void setEchoReplyEnabled(bool enabled) { echo_reply_enabled_ = enabled; }

  bool handle(const IPv4Address& src, const IPv4Address& dst,
              uint8_t protocol, std::span<const std::byte> payload) override;

  IPv4Dispatcher& dispatcher() { return *this; }

private:
  IPv4Protocol& ipv4_;
  bool echo_reply_enabled_{false};
};