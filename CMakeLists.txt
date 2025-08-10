cmake_minimum_required(VERSION 3.16)
project(EthernetNetworkingStack)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Find required packages
find_package(PkgConfig REQUIRED)

# Include directories
include_directories(include)
include_directories(src)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(networking_stack ${SOURCES})

# Link libraries
target_link_libraries(networking_stack pthread)

# Install target
install(TARGETS networking_stack DESTINATION bin)