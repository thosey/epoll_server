cmake_minimum_required(VERSION 3.10)
project(echo_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable more warnings
add_compile_options(-Wall -Wextra -pedantic -O2)

# Add the executable
add_executable(echo_server
    src/main.cpp
    src/ListeningSocket.cpp
    src/Epoll.cpp
    src/SocketRAII.cpp
)
# Link against OpenSSL (optional, for future TLS support)
find_package(OpenSSL QUIET)
if (OPENSSL_FOUND)
    target_compile_definitions(echo_server PRIVATE USE_OPENSSL)
    target_include_directories(echo_server PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(echo_server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()