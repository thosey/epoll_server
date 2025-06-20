#pragma once

#include "SocketRAII.hpp"
#include "Utils.hpp"
#include <span>
#include <sys/epoll.h>
#include <vector>

class Epoll {
    SocketRAII epoll_fd;
    int server_fd;
    std::vector<epoll_event> events;
    int num_ready = 0;

  public:
    enum Mode { Once, Indefinitely };

    explicit Epoll(int server_fd, int max_events = 1024);
    void processEvents(Mode mode);

  private:
    void acceptNewConnections();
    void handleClientData(int fd);
    void add(int client_fd);
    void remove(int fd);
    std::span<epoll_event> collectPendingEvents();
};
