#include "EpollServer.hpp"
#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <string>

constexpr int PORT = 8080;

EpollServer::EpollServer(int max_events)
    : epoll_fd(epoll_create1(0)), //
      server_fd(PORT),               //
      events(max_events) {

    if (epoll_fd == -1) {
        throw std::runtime_error("epoll_create1() failed");
    }
    add(server_fd);
}

void EpollServer::add(int client_fd) {
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        throw std::runtime_error("epoll_ctl() add fd failed");
    }
}

void EpollServer::modify(int fd, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::runtime_error("epoll_ctl() mod fd failed");
    }
}

void EpollServer::remove(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::runtime_error("epoll_ctl() remove fd failed");
    }
    close(fd);
    outBuffers.erase(fd); 
}

std::span<epoll_event> EpollServer::collectPendingEvents() {
    num_ready = epoll_wait(epoll_fd, events.data(), events.size(), -1);
    if (num_ready < 0)
        throw std::runtime_error("epoll_wait() failed");
    return std::span(events.data(), num_ready);
}

void EpollServer::acceptNewConnections() {
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            throw std::runtime_error("accept() failed");
        }
        set_non_blocking(client_fd);
        add(client_fd);
    }
}

void EpollServer::handleClientData(int fd) {
    char buffer[4096];
    while (true) {
        ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            remove(fd);
            break;
        } else if (count == 0) {
            remove(fd);
            break;
        } else {
            // If there's already unsent data, just buffer this new data
            if (!outBuffers[fd].empty()) {
                outBuffers[fd].append(buffer, count);
                modify(fd, EPOLLIN | EPOLLOUT | EPOLLET);
                continue;
            }
            // Try to send immediately
            ssize_t sent = send(fd, buffer, count, 0);
            if (sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    sent = 0;
                } else {
                    remove(fd);
                    break;
                }
            }
            if (sent < count) {
                // Buffer the rest
                outBuffers[fd].append(buffer + sent, count - sent);
                modify(fd, EPOLLIN | EPOLLOUT | EPOLLET);
            }
        }
    }
}

void EpollServer::finishClientWrite(int fd) {
    auto &buf = outBuffers[fd];
    while (!buf.empty()) {
        ssize_t sent = send(fd, buf.data(), buf.size(), 0);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            remove(fd);
            return;
        }
        buf.erase(0, sent);
        // If we sent less than the buffer, break to avoid busy waiting
        // and allow other events to be processed. 
        if (sent < static_cast<ssize_t>(buf.size()))
            break;
    }
    if (buf.empty()) {
        // No more data to write, disable EPOLLOUT
        modify(fd, EPOLLIN | EPOLLET);
        outBuffers.erase(fd);
    }
}

void EpollServer::processEvents(Mode mode) {
    do {
        for (const auto &event : collectPendingEvents()) {
            int fd = event.data.fd;
            if (fd == server_fd) {
                acceptNewConnections();
                continue;
            }
            if (event.events & EPOLLOUT) {
                finishClientWrite(fd);
            }
            if (event.events & EPOLLIN) {
                handleClientData(fd);
            }
        }
    } while (mode == Indefinitely);
}
