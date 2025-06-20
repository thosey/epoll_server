#pragma once

#include "SocketRAII.hpp"
#include "ListeningSocket.hpp"
#include "Utils.hpp"
#include <span>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

class EpollServer {
    SocketRAII epoll_fd;
    ListeningSocket server_fd;
    std::vector<epoll_event> events;
    int num_ready = 0;
    std::unordered_map<int, std::string> outBuffers; 

  public:
    enum Mode { Once, Indefinitely };

    explicit EpollServer(int max_events = 1024);
    void processEvents(Mode mode);

  private:
    void acceptNewConnections();
    void handleClientData(int fd);
    void handleClientWrite(int fd); 
    void add(int client_fd);
    void modify(int fd, uint32_t events); 
    void remove(int fd);
    std::span<epoll_event> collectPendingEvents();
};
