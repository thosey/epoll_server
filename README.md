# 🖧 Epoll-Based C++ Echo Server

A high-performance echo server implemented in modern C++ using non-blocking sockets and the Linux `epoll` API. Designed to be educational, efficient, and extendable — perfect for learning systems-level networking or as a foundation for real-world network services.

## Features

-  Efficient I/O with `epoll` (edge-triggered)
-  Non-blocking sockets
-  Handles many concurrent clients

## Requirements

- Linux or WSL (for `epoll` support)
- C++20 or later
- CMake (optional, but recommended for builds)

## Build & Run

mkdir build && cd build
cmake ..
make
./echo_server

