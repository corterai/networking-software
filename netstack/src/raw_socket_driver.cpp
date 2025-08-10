#include "raw_socket_driver.hpp"
#include "eth.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

static std::optional<MacAddress> parse_mac(const uint8_t* p) {
  MacAddress m{};
  for (int i = 0; i < 6; ++i) m.bytes[i] = static_cast<std::byte>(p[i]);
  return m;
}

std::optional<MacAddress> MacAddress::fromString(const std::string& s) {
  unsigned int b[6];
  if (std::sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
    return std::nullopt;
  }
  MacAddress m{};
  for (int i = 0; i < 6; ++i) m.bytes[i] = static_cast<std::byte>(b[i] & 0xFFu);
  return m;
}

std::string MacAddress::toString() const {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                std::to_integer<unsigned>(bytes[0]),
                std::to_integer<unsigned>(bytes[1]),
                std::to_integer<unsigned>(bytes[2]),
                std::to_integer<unsigned>(bytes[3]),
                std::to_integer<unsigned>(bytes[4]),
                std::to_integer<unsigned>(bytes[5]));
  return std::string(buf);
}

bool RawSocketDriver::open(const std::string& ifname, const std::optional<MacAddress>& override_mac) {
  fd_ = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (fd_ < 0) {
    std::perror("socket");
    return false;
  }

  struct ifreq ifr{};
  std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname.c_str());
  if (ioctl(fd_, SIOCGIFINDEX, &ifr) < 0) {
    std::perror("ioctl(SIOCGIFINDEX)");
    return false;
  }
  ifindex_ = ifr.ifr_ifindex;

  if (override_mac.has_value()) {
    own_mac_ = *override_mac;
  } else {
    if (ioctl(fd_, SIOCGIFHWADDR, &ifr) < 0) {
      std::perror("ioctl(SIOCGIFHWADDR)");
      return false;
    }
    auto mac = parse_mac(reinterpret_cast<uint8_t*>(ifr.ifr_hwaddr.sa_data));
    if (!mac.has_value()) return false;
    own_mac_ = *mac;
  }

  sockaddr_ll sll{};
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = ifindex_;
  sll.sll_protocol = htons(ETH_P_ALL);
  if (bind(fd_, reinterpret_cast<sockaddr*>(&sll), sizeof(sll)) < 0) {
    std::perror("bind");
    return false;
  }

  return true;
}

void RawSocketDriver::close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

void RawSocketDriver::setReceiveHandler(ReceiveHandler handler) { handler_ = std::move(handler); }

void RawSocketDriver::send(const EthFrame& frame) {
  if (fd_ < 0) return;
  std::vector<std::byte> buf;
  buf.reserve(14 + frame.payload.size());
  for (int i = 0; i < 6; ++i) buf.push_back(frame.dst.bytes[i]);
  for (int i = 0; i < 6; ++i) buf.push_back(frame.src.bytes[i]);
  uint16_t et = htons(static_cast<uint16_t>(frame.ethertype));
  buf.push_back(static_cast<std::byte>(et >> 8));
  buf.push_back(static_cast<std::byte>(et & 0xFF));
  buf.insert(buf.end(), frame.payload.begin(), frame.payload.end());

  sockaddr_ll sll{};
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = ifindex_;
  sll.sll_halen = ETH_ALEN;
  for (int i = 0; i < 6; ++i) sll.sll_addr[i] = std::to_integer<unsigned char>(frame.dst.bytes[i]);

  (void) ::sendto(fd_, buf.data(), buf.size(), 0, reinterpret_cast<sockaddr*>(&sll), sizeof(sll));
}

void RawSocketDriver::pollOnce() {
  if (fd_ < 0) return;
  std::array<std::byte, 2048> buffer{};
  ssize_t n = ::recv(fd_, buffer.data(), buffer.size(), 0);
  if (n <= 0) return;
  if (static_cast<size_t>(n) < 14) return;

  EthFrame f{};
  for (int i = 0; i < 6; ++i) f.dst.bytes[i] = buffer[static_cast<size_t>(i)];
  for (int i = 0; i < 6; ++i) f.src.bytes[i] = buffer[static_cast<size_t>(6 + i)];
  uint16_t et = (static_cast<uint16_t>(std::to_integer<unsigned>(buffer[12])) << 8) |
                static_cast<uint16_t>(std::to_integer<unsigned>(buffer[13]));
  f.ethertype = static_cast<EthType>(et);
  f.payload.assign(buffer.begin() + 14, buffer.begin() + n);

  if (handler_) handler_(f);
}