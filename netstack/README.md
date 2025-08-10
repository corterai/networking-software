A minimal userspace C++ Ethernet/ARP/IPv4/ICMP/UDP scaffold using AF_PACKET raw sockets.

Build:

  mkdir -p build && cd build && cmake .. && cmake --build . -j

Run (requires CAP_NET_RAW or root):

  sudo ./netstack <iface> <ipv4> [mac]

It will:
- Answer ARP requests for the provided IPv4 using the interface MAC (or provided MAC)
- Parse IPv4 and dispatch ICMP and UDP
- Print basic UDP packet info

Note: ICMP echo replies are parsed and prepared but L2 resolution for transmit is left to the caller; you can wire ARP resolution and call IPv4Protocol::send with destination MAC.
