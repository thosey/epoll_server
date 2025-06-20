// Epoll.cpp
#include "Epoll.hpp"
#include <stdexcept>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <errno.h>

Epoll::Epoll(int max_events)
    : epoll_fd(epoll_create1(0)), events(max_events) {
    if (epoll_fd == -1) throw std::runtime_error("epoll_create1() failed");
}

void Epoll::add(int client_fd) {
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
        throw std::runtime_error("epoll_ctl() add fd failed");
}

void Epoll::remove(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1)
        throw std::runtime_error("epoll_ctl() remove fd failed");
}

std::span<epoll_event> Epoll::wait() {
    num_ready = epoll_wait(epoll_fd, events.data(), events.size(), -1);
    if (num_ready < 0) throw std::runtime_error("epoll_wait() failed");
    return std::span(events.data(), num_ready);
}

void Epoll::acceptNewConnections(int server_fd) {
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            throw std::runtime_error("accept() failed");
        }
        set_non_blocking(client_fd);
        add(client_fd);
    }
}

void Epoll::handleClientData(int fd) {
    char buffer[4096];
    while (true) {
        ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            remove(fd);
            close(fd);
            break;
        } else if (count == 0) {
            remove(fd);
            close(fd);
            break;
        } else {
            send(fd, buffer, count, 0);
        }
    }
}

void Epoll::processEvents(int server_fd, Mode mode) {
    do {
        for (const auto& event : wait()) {
            int fd = event.data.fd;
            if (fd == server_fd) {
                acceptNewConnections(server_fd);
            } else {
                handleClientData(fd);
            }
        }
    } while (mode == Indefinitely);
}
