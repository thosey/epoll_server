#include "ListeningSocket.hpp"
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

ListeningSocket::ListeningSocket(uint16_t port)
    : fd(::socket(AF_INET, SOCK_STREAM, 0)) {
    if (fd == -1)
        throw std::runtime_error("socket() failed");
    set_socket_options();
    bind_to_port(port);
    set_non_blocking(fd);
    if (listen(fd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");
}

void ListeningSocket::bind_to_port(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");
}

void ListeningSocket::set_socket_options() {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");
}

ListeningSocket::operator int() const { return fd; }
