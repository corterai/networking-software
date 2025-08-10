#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

inline constexpr uint16_t to_be16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }
inline constexpr uint16_t from_be16(uint16_t v) { return to_be16(v); }
inline constexpr uint32_t bswap32(uint32_t v) { return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v >> 24) & 0xFF); }
inline constexpr uint32_t to_be32(uint32_t v) { return bswap32(v); }
inline constexpr uint32_t from_be32(uint32_t v) { return bswap32(v); }

inline uint16_t checksum16(const std::byte* data, size_t len) {
  uint32_t sum = 0;
  for (size_t i = 0; i + 1 < len; i += 2) {
    uint16_t word = (static_cast<uint16_t>(std::to_integer<unsigned>(data[i])) << 8) |
                    static_cast<uint16_t>(std::to_integer<unsigned>(data[i + 1]));
    sum += word;
    if (sum > 0xFFFF) sum = (sum & 0xFFFF) + (sum >> 16);
  }
  if (len & 1) {
    uint16_t word = static_cast<uint16_t>(std::to_integer<unsigned>(data[len - 1])) << 8;
    sum += word;
    if (sum > 0xFFFF) sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return static_cast<uint16_t>(~sum & 0xFFFF);
}