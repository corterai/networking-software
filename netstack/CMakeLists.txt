cmake_minimum_required(VERSION 3.16)
project(netstack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
  -Wdouble-promotion -Wformat=2 -Wundef -fno-exceptions -fno-rtti
)

add_executable(netstack
  src/main.cpp
  src/raw_socket_driver.cpp
  src/arp.cpp
  src/ipv4.cpp
  src/icmp.cpp
  src/udp.cpp
)

target_include_directories(netstack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(netstack PRIVATE Threads::Threads)