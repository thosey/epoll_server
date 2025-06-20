# 🖧 Epoll-Based C++ Echo Server

A high-performance echo server implemented in modern C++ using non-blocking sockets and the Linux `epoll` API. Designed to be educational, efficient, and extendable — perfect for learning systems-level networking or as a foundation for real-world network services.

## Features

-  Efficient I/O with `epoll` (edge-triggered)
-  Non-blocking sockets
-  RAII-style resource management
-  Clear modular structure
-  Handles many concurrent clients
-  Easily extensible (e.g. TLS, timeouts, logging)

## Requirements

- Linux or WSL (for `epoll` support)
- C++17 or later
- CMake (optional, but recommended for builds)
- OpenSSL (optional, for future TLS support)

## Build & Run

mkdir build && cd build
cmake ..
make
./echo_server

