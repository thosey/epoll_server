#pragma once

#include "SocketRAII.hpp"
#include "Utils.hpp"
#include <cstdint>

class ListeningSocket {
    SocketRAII fd;
    void bind_to_port(uint16_t port);
    void set_socket_options();

  public:
    explicit ListeningSocket(uint16_t port);
    operator int() const;
};
