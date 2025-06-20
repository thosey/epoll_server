#pragma once

#include "SocketRAII.hpp"
#include "Utils.hpp"
#include <vector>
#include <sys/epoll.h>
#include <span>

class Epoll {
    SocketRAII epoll_fd;
    std::vector<epoll_event> events;
    int num_ready = 0;

public:
    enum Mode { Once, Indefinitely };

    explicit Epoll(int max_events = 1024);
    void add(int client_fd);
    void remove(int fd);
    std::span<epoll_event> wait();
    void accept_new_connections(int server_fd);
    void handle_client_data(int fd);
    void process_events(int server_fd, Mode mode);
};
