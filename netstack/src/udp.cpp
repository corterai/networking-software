#include "udp.hpp"
#include "eth.hpp"

static uint16_t read16(std::span<const std::byte> b, size_t i) {
  return static_cast<uint16_t>((std::to_integer<unsigned>(b[i]) << 8) |
                               std::to_integer<unsigned>(b[i+1]));
}

bool UDPProtocol::handle(const IPv4Address& src, const IPv4Address& dst,
                         uint8_t protocol, std::span<const std::byte> payload) {
  (void)dst; (void)protocol;
  if (payload.size() < 8) return false;
  uint16_t src_port = read16(payload, 0);
  uint16_t dst_port = read16(payload, 2);
  uint16_t length = read16(payload, 4);
  if (payload.size() < length || length < 8) return false;
  std::span<const std::byte> data{payload.data() + 8, static_cast<size_t>(length - 8)};
  if (handler_) return handler_(src, src_port, dst_port, data);
  return false;
}