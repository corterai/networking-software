# Netstack (C++) — Minimal Userspace Ethernet/ARP/IPv4/ICMP/UDP Stack

## Overview
Netstack is a minimal, educational networking stack implemented in modern C++ that operates in Linux userspace using raw AF_PACKET sockets. It crafts and parses Ethernet frames and implements basic ARP, IPv4, ICMP (echo request/reply), and UDP receive handling. It is designed to run atop any standard Ethernet NIC/controller available on Linux.

Primary goals:
- Demonstrate how to interact with a NIC at Layer 2 using raw sockets
- Provide clear, readable C++ implementations of ARP, IPv4, ICMP, and UDP parsing
- Offer a small set of extension points (transmit callback, MAC resolution callback, protocol dispatchers)

Non-goals (for now): TCP, fragmentation/reassembly, routing, checksums beyond what’s necessary for ICMP/IPv4 header, or production hardening.

## Features
- Listens on a specified network interface and answers ARP requests for a configured IPv4.
- Parses IPv4 packets and dispatches to ICMP and UDP handlers.
- ICMP echo-request (ping) handling with automatic echo-reply if the peer MAC is known via ARP cache.
- UDP parser that invokes a user callback with source/destination ports and payload.

## Architecture
- `RawSocketDriver`: Minimal AF_PACKET driver. Opens/binds a raw socket to an interface, sends/receives Ethernet frames, and exposes the interface MAC.
- `Eth helpers` (`eth.hpp`): Byte-order helpers and checksum routine used by IPv4/ICMP.
- `ARP` (`arp.hpp/.cpp`): Parses ARP requests/replies, serializes replies, and maintains a simple in-memory ARP cache.
- `IPv4` (`ipv4.hpp/.cpp`): Parses and constructs IPv4 packets. Provides:
  - `setTransmitCallback`: to emit L3 payloads wrapped in Ethernet by the caller
  - `setMacResolver`: to resolve destination MAC for a given IPv4
  - `receive`: dispatches payload to protocol handlers (ICMP/UDP)
  - `send`: builds an IPv4 packet and invokes the transmit callback
- `ICMP` (`icmp.hpp/.cpp`): Dispatcher that forms echo replies and sends them if MAC can be resolved.
- `UDP` (`udp.hpp/.cpp`): Dispatcher that parses UDP headers and invokes a user receive callback.
- `main.cpp`: Wires everything together, sets callbacks, updates ARP cache, and runs the poll loop.

### Packet flow
1. NIC -> `RawSocketDriver::pollOnce()` -> Ethernet frame
2. If ARP: parse ARP; update ARP cache; reply to requests for own IP
3. If IPv4: `IPv4Protocol::receive()` parses header and dispatches by protocol
   - ICMP: If echo request and MAC is known, send echo reply
   - UDP: Invoke user callback to inspect payload
4. For outbound IPv4: `IPv4Protocol::send()` builds header and calls transmit callback
5. Transmit callback wraps IPv4 payload in Ethernet and sends via `RawSocketDriver::send()`

## Build
Requirements (on your machine):
- A C++20 compiler (e.g., g++ 10+, clang 12+)
- CMake 3.16+

Build steps:
```bash
cd /workspace/netstack
mkdir -p build && cd build
cmake ..
make -j
```
This will produce `./netstack` under `build/`.

## Run
Raw sockets require elevated privileges. Either run as root, or grant the binary the `CAP_NET_RAW` capability.

- As root:
```bash
sudo ./netstack <iface> <ipv4> [mac]
```
Examples:
```bash
sudo ./netstack eth0 192.168.1.50
sudo ./netstack eth0 10.0.0.10 aa:bb:cc:dd:ee:ff
```

- Or with capabilities (after building):
```bash
sudo setcap cap_net_raw+ep ./netstack
./netstack <iface> <ipv4> [mac]
```

Behavior:
- Answers ARP who-has for `<ipv4>` using the interface MAC (or the specified `[mac]`).
- Prints basic info for received UDP packets.
- Replies to ICMP echo requests (pings) when the sender’s MAC is known (learned via ARP).

## CLI
```
Usage: netstack <iface> <ipv4> [mac]
  <iface>  Network interface name (e.g., eth0)
  <ipv4>   IPv4 address to claim/answer for (e.g., 192.168.1.50)
  [mac]    Optional MAC to use instead of the interface’s MAC
```

## Code structure
```
netstack/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    eth.hpp
    raw_socket_driver.hpp
    raw_socket_driver.cpp
    arp.hpp
    arp.cpp
    ipv4.hpp
    ipv4.cpp
    icmp.hpp
    icmp.cpp
    udp.hpp
    udp.cpp
```

## API quick reference
- `RawSocketDriver`
  - `bool open(const std::string& ifname, std::optional<MacAddress> override_mac)`
  - `void setReceiveHandler(std::function<void(const EthFrame&)>)`
  - `void send(const EthFrame& frame)`
  - `void pollOnce()`
  - `MacAddress macAddress() const`

- `IPv4Protocol`
  - `void setTransmitCallback(IPv4TxCallback)` — wrap and send IPv4 payloads via Ethernet
  - `void setMacResolver(MacResolverCallback)` — resolve destination MAC by IPv4
  - `void receive(span<const byte> packet, filter, icmp_dispatcher, udp_dispatcher)`
  - `void send(const IPv4Address& dst, uint8_t proto, span<const byte> payload, const MacAddress& dst_mac)`

- `ICMPProtocol`
  - `void setEchoReplyEnabled(bool)`
  - `bool handle(...)` — dispatcher used by `IPv4Protocol`

- `UDPProtocol`
  - `void setReceiveHandler(ReceiveHandler)` — called with `(src_ip, src_port, dst_port, payload)`
  - `bool handle(...)` — dispatcher used by `IPv4Protocol`

- `ArpCache`
  - `void update(const IPv4Address&, const MacAddress&)`
  - `std::optional<MacAddress> lookup(const IPv4Address&) const`

## Limitations and notes
- No IP fragmentation/reassembly.
- No TCP.
- No routing or forwarding; only handles traffic destined to the configured IPv4.
- Minimal checksum handling (IPv4 header and ICMP echo). UDP checksum is parsed but not validated/transmitted.
- ARP is passive (learns from seen traffic). No active ARP resolution is performed out of the box.
- MTU, VLAN, and offloads are not explicitly handled; this is a learning scaffold.

## Extending the stack
- Active ARP resolution: add a small ARP-request sender and a pending-send queue, then call `ipv4.send(...)` once the reply arrives and the cache is updated.
- UDP transmit: implement a small helper to build a UDP header+payload, compute checksum (optional), then call `ipv4.send(...)` with the destination MAC from ARP cache.
- Add more protocols: create another `IPv4Dispatcher` implementation (e.g., for protocol 6/TCP placeholder) and wire it into `IPv4Protocol::receive` dispatch.
- Improve safety: validate checksums, lengths, and add robust error handling and logging.

## Troubleshooting
- Permission errors (`EPERM`/`Operation not permitted`): run as root or grant `CAP_NET_RAW` to the binary.
- No packets seen: ensure the interface `<iface>` is up, in the correct network, and not blocked by firewall rules. Use `tcpdump -i <iface> -nn -e` in another terminal to confirm traffic.
- ARP not answered: verify you used the interface actually connected to the network where peers are sending ARP who-has. Check that the `<ipv4>` is unique on the network.
- Ping does not get replies: the ARP cache must have the sender’s MAC; try initiating any traffic from the peer (e.g., ARP who-has), or implement active ARP requests.

## License
This repository is provided as-is for educational purposes; adapt and license according to your needs.
