#include "arp.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>

static bool parse_ipv4(const std::string& s, uint32_t& out_be) {
  in_addr a{};
  if (::inet_pton(AF_INET, s.c_str(), &a) != 1) return false;
  out_be = a.s_addr; // already network byte order
  return true;
}

std::optional<IPv4Address> IPv4Address::fromString(const std::string& s) {
  uint32_t be{};
  if (!parse_ipv4(s, be)) return std::nullopt;
  return IPv4Address{be};
}

std::string IPv4Address::toString() const {
  char buf[INET_ADDRSTRLEN];
  in_addr a{}; a.s_addr = be;
  const char* p = ::inet_ntop(AF_INET, &a, buf, sizeof(buf));
  return p ? std::string(p) : std::string("0.0.0.0");
}

std::optional<ArpPacket> ArpPacket::parse(std::span<const std::byte> data) {
  if (data.size() < 28) return std::nullopt;
  auto u = [&](size_t i){ return std::to_integer<unsigned>(data[i]); };
  uint16_t htype = (u(0) << 8) | u(1);
  uint16_t ptype = (u(2) << 8) | u(3);
  uint8_t hlen = u(4);
  uint8_t plen = u(5);
  uint16_t oper = (u(6) << 8) | u(7);
  if (htype != 1 || ptype != 0x0800 || hlen != 6 || plen != 4) return std::nullopt;
  if (data.size() < 8 + 2*hlen + 2*plen) return std::nullopt;
  ArpPacket p{};
  p.operation = static_cast<ArpOperation>(oper);
  for (int i = 0; i < 6; ++i) p.sender_mac.bytes[i] = data[8 + i];
  p.sender_ip.be = (u(14) << 24) | (u(15) << 16) | (u(16) << 8) | u(17);
  for (int i = 0; i < 6; ++i) p.target_mac.bytes[i] = data[18 + i];
  p.target_ip.be = (u(24) << 24) | (u(25) << 16) | (u(26) << 8) | u(27);
  return p;
}

std::vector<std::byte> ArpPacket::serialize() const {
  std::vector<std::byte> b;
  b.reserve(28);
  auto push16 = [&](uint16_t v){ b.push_back(static_cast<std::byte>(v >> 8)); b.push_back(static_cast<std::byte>(v & 0xFF)); };
  auto push8 = [&](uint8_t v){ b.push_back(static_cast<std::byte>(v)); };
  push16(1); // htype Ethernet
  push16(0x0800); // IPv4
  push8(6); // hlen
  push8(4); // plen
  push16(static_cast<uint16_t>(operation));
  for (int i = 0; i < 6; ++i) b.push_back(sender_mac.bytes[i]);
  b.push_back(static_cast<std::byte>(sender_ip.be >> 24));
  b.push_back(static_cast<std::byte>((sender_ip.be >> 16) & 0xFF));
  b.push_back(static_cast<std::byte>((sender_ip.be >> 8) & 0xFF));
  b.push_back(static_cast<std::byte>(sender_ip.be & 0xFF));
  for (int i = 0; i < 6; ++i) b.push_back(target_mac.bytes[i]);
  b.push_back(static_cast<std::byte>(target_ip.be >> 24));
  b.push_back(static_cast<std::byte>((target_ip.be >> 16) & 0xFF));
  b.push_back(static_cast<std::byte>((target_ip.be >> 8) & 0xFF));
  b.push_back(static_cast<std::byte>(target_ip.be & 0xFF));
  return b;
}

ArpPacket ArpPacket::makeReply(const MacAddress& own_mac, const IPv4Address& own_ip,
                               const MacAddress& target_mac, const IPv4Address& target_ip) {
  ArpPacket r{};
  r.operation = ArpOperation::Reply;
  r.sender_mac = own_mac;
  r.sender_ip = own_ip;
  r.target_mac = target_mac;
  r.target_ip = target_ip;
  return r;
}