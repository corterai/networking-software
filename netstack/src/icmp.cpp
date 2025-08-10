#include "icmp.hpp"
#include "eth.hpp"
#include <vector>

bool ICMPProtocol::handle(const IPv4Address& src, const IPv4Address& dst,
                          uint8_t protocol, std::span<const std::byte> payload) {
  (void)dst; (void)protocol;
  if (payload.size() < 8) return false;
  uint8_t type = std::to_integer<unsigned>(payload[0]);
  uint8_t code = std::to_integer<unsigned>(payload[1]);
  if (type == 8 && code == 0 && echo_reply_enabled_) {
    std::vector<std::byte> reply(payload.begin(), payload.end());
    reply[0] = std::byte{0}; // echo reply
    reply[2] = std::byte{0}; reply[3] = std::byte{0};
    uint16_t cksum = checksum16(reply.data(), reply.size());
    reply[2] = static_cast<std::byte>(cksum >> 8);
    reply[3] = static_cast<std::byte>(cksum & 0xFF);

    auto mac = ipv4_.resolveMac(src);
    if (!mac.has_value()) return true; // handled, but cannot send without MAC
    ipv4_.send(src, static_cast<uint8_t>(IPv4ProtocolNumber::ICMP), reply, *mac);
    return true;
  }
  return false;
}