#include "EpollServer.hpp"
#include "ListeningSocket.hpp"
#include <iostream>

int main() {
    try {
        EpollServer epoll{};
        epoll.processEvents(EpollServer::Indefinitely);
    } catch (const std::exception &ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
