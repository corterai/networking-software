#include "ipv4.hpp"
#include <iostream>

struct IPv4Header {
  uint8_t ver_ihl{};
  uint8_t dscp_ecn{};
  uint16_t total_length{};
  uint16_t identification{};
  uint16_t flags_fragment{};
  uint8_t ttl{};
  uint8_t protocol{};
  uint16_t header_checksum{};
  uint32_t src{};
  uint32_t dst{};
};

static bool parse_ipv4_header(std::span<const std::byte> data, IPv4Header& h) {
  if (data.size() < 20) return false;
  auto u = [&](size_t i){ return std::to_integer<unsigned>(data[i]); };
  h.ver_ihl = static_cast<uint8_t>(u(0));
  h.dscp_ecn = static_cast<uint8_t>(u(1));
  h.total_length = static_cast<uint16_t>((u(2) << 8) | u(3));
  h.identification = static_cast<uint16_t>((u(4) << 8) | u(5));
  h.flags_fragment = static_cast<uint16_t>((u(6) << 8) | u(7));
  h.ttl = static_cast<uint8_t>(u(8));
  h.protocol = static_cast<uint8_t>(u(9));
  h.header_checksum = static_cast<uint16_t>((u(10) << 8) | u(11));
  h.src = (u(12) << 24) | (u(13) << 16) | (u(14) << 8) | u(15);
  h.dst = (u(16) << 24) | (u(17) << 16) | (u(18) << 8) | u(19);
  uint8_t ihl = h.ver_ihl & 0x0F;
  if (ihl < 5) return false;
  if (data.size() < static_cast<size_t>(ihl*4)) return false;
  return true;
}

void IPv4Protocol::receive(std::span<const std::byte> packet,
                           std::function<IPv4Accept(const IPv4Address& dst)> filter,
                           IPv4Dispatcher& icmp_disp,
                           IPv4Dispatcher& udp_disp) {
  IPv4Header h{};
  if (!parse_ipv4_header(packet, h)) return;
  const uint8_t ihl = h.ver_ihl & 0x0F;
  const size_t header_len = static_cast<size_t>(ihl * 4);
  if (packet.size() < header_len || h.total_length < header_len) return;
  const size_t payload_len = static_cast<size_t>(h.total_length) - header_len;
  if (packet.size() < header_len + payload_len) return;
  std::span<const std::byte> payload{packet.data() + header_len, payload_len};

  IPv4Address src{h.src};
  IPv4Address dst{h.dst};
  if (filter(dst) == IPv4Accept::Drop) return;

  switch (static_cast<IPv4ProtocolNumber>(h.protocol)) {
    case IPv4ProtocolNumber::ICMP:
      icmp_disp.handle(src, dst, h.protocol, payload);
      break;
    case IPv4ProtocolNumber::UDP:
      udp_disp.handle(src, dst, h.protocol, payload);
      break;
    default:
      break;
  }
}

void IPv4Protocol::send(const IPv4Address& dst, uint8_t protocol,
                        std::span<const std::byte> payload,
                        const MacAddress& dst_mac) {
  std::vector<std::byte> buf;
  const uint8_t ihl = 5;
  const uint16_t total_length = static_cast<uint16_t>(ihl*4 + payload.size());
  buf.resize(ihl*4);
  buf[0] = static_cast<std::byte>((4 << 4) | ihl);
  buf[1] = std::byte{0};
  buf[2] = static_cast<std::byte>(total_length >> 8);
  buf[3] = static_cast<std::byte>(total_length & 0xFF);
  buf[4] = std::byte{0}; buf[5] = std::byte{1}; // identification
  buf[6] = std::byte{0}; buf[7] = std::byte{0}; // flags/frag
  buf[8] = std::byte{64}; // ttl
  buf[9] = static_cast<std::byte>(protocol);
  buf[10] = std::byte{0}; buf[11] = std::byte{0}; // checksum placeholder
  buf[12] = static_cast<std::byte>(own_ip_.be >> 24);
  buf[13] = static_cast<std::byte>((own_ip_.be >> 16) & 0xFF);
  buf[14] = static_cast<std::byte>((own_ip_.be >> 8) & 0xFF);
  buf[15] = static_cast<std::byte>(own_ip_.be & 0xFF);
  buf[16] = static_cast<std::byte>(dst.be >> 24);
  buf[17] = static_cast<std::byte>((dst.be >> 16) & 0xFF);
  buf[18] = static_cast<std::byte>((dst.be >> 8) & 0xFF);
  buf[19] = static_cast<std::byte>(dst.be & 0xFF);

  uint16_t cksum = checksum16(buf.data(), buf.size());
  buf[10] = static_cast<std::byte>(cksum >> 8);
  buf[11] = static_cast<std::byte>(cksum & 0xFF);

  buf.insert(buf.end(), payload.begin(), payload.end());

  if (tx_) tx_(dst_mac, buf);
}