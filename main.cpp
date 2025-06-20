#include "ListeningSocket.hpp"
#include "Epoll.hpp"
#include <iostream>

constexpr int PORT = 8080;

int main() {
    try {
        ListeningSocket server(PORT);
        Epoll epoll;
        epoll.add(server);
        epoll.processEvents(server, Epoll::Indefinitely);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
