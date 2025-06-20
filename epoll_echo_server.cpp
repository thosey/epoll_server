/**
 * @file epoll_echo_server.cpp
 * @brief High-performance TCP echo server using epoll and non-blocking sockets.
 */

#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <stdexcept>

/// The TCP port this server listens on.
constexpr int PORT = 8080;
/// Maximum number of events to handle per epoll_wait call.
constexpr int MAX_EVENTS = 1024;
/// Buffer size for reading client data.
constexpr int BUFFER_SIZE = 4096;

namespace {

/**
 * @brief Sets a file descriptor to non-blocking mode.
 *
 * This utility is useful when working with asynchronous or event-driven I/O,
 * such as epoll or select. It modifies the file descriptor flags using fcntl
 * to include O_NONBLOCK.
 *
 * @param fd File descriptor to modify.
 * @throws std::runtime_error if the operation fails.
 */
void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

void accept_new_connections(int server_fd, Epoll& epoll);
void handle_client_data(int fd, Epoll& epoll);

} // anonymous namespace

class SocketRAII {
    int fd;
public:
    explicit SocketRAII(int fd) : fd(fd) {}
    ~SocketRAII() { if (fd != -1) close(fd); }
    SocketRAII(const SocketRAII&) = delete;
    SocketRAII& operator=(const SocketRAII&) = delete;
    operator int() const { return fd; }
    int release() { int temp = fd; fd = -1; return temp; }
};

/**
 * @class ListeningSocket
 * @brief Wrapper around a TCP socket set up to listen for incoming connections.
 *
 * This class encapsulates the creation, configuration, binding, and listening stages
 * of a TCP server socket. It ensures non-blocking behavior and uses RAII for resource management.
 * Once constructed, it can be monitored with epoll to accept new client connections.
 */
class ListeningSocket {
    SocketRAII fd;

    void bind_to_port(uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");
    }

    void set_socket_options() {
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("setsockopt() failed");
    }

public:
    explicit ListeningSocket(uint16_t port)
        : fd(::socket(AF_INET, SOCK_STREAM, 0)) {
        if (fd == -1) throw std::runtime_error("socket() failed");
        set_socket_options();
        bind_to_port(port);
        set_non_blocking(fd);
        if (listen(fd, SOMAXCONN) < 0)
            throw std::runtime_error("listen() failed");
    }

    operator int() const { return fd; }
};

/**
 * @class Epoll
 * @brief Thin RAII wrapper around the Linux epoll API for I/O event notification.
 *
 * This class allows registration of file descriptors and waits for I/O readiness events.
 * It uses `epoll_create1`, `epoll_ctl`, and `epoll_wait` to efficiently monitor many
 * file descriptors. The `wait()` function returns a span of `epoll_event` entries,
 * enabling use with C++ range-based for loops.
 *
 * Typically used for building scalable network servers that respond to many clients.
 */
class Epoll {
    SocketRAII epoll_fd;
    std::vector<epoll_event> events;
    int num_ready = 0;

public:
    explicit Epoll(int max_events = MAX_EVENTS)
        : epoll_fd(epoll_create1(0)), events(max_events) {
        if (epoll_fd == -1) throw std::runtime_error("epoll_create1() failed");
    }

    /**
     * @brief Registers a client socket file descriptor with epoll for read events.
     *
     * This method sets up edge-triggered monitoring (EPOLLET) for input (EPOLLIN) on the
     * given client file descriptor so that epoll can notify when the socket becomes readable.
     *
     * @param client_fd A client socket file descriptor to be watched for input readiness.
     */
    void add(int client_fd) {
        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
            throw std::runtime_error("epoll_ctl() add fd failed");
    }

    /**
     * @brief Removes a file descriptor from being monitored by epoll.
     *
     * This should be called when a client disconnects or when you no longer
     * wish to monitor the file descriptor for events.
     *
     * @param fd The file descriptor to remove from the epoll instance.
     */
    void remove(int fd) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1)
            throw std::runtime_error("epoll_ctl() remove fd failed");
    }

    /**
     * @brief Waits for I/O events on registered file descriptors.
     *
     * This method blocks until one or more file descriptors are ready.
     * It returns a span over the list of `epoll_event` structures that
     * describe the ready file descriptors. These events can be iterated
     * over using a range-based for loop:
     *
     * @code
     * for (const auto& event : epoll.wait()) {
     *     int fd = event.data.fd;
     *     // Handle event on fd...
     * }
     * @endcode
     *
     * @return std::span<epoll_event> Span of ready events.
     */
    auto wait() {
        num_ready = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (num_ready < 0) throw std::runtime_error("epoll_wait() failed");
        return std::span(events.data(), num_ready);
    }
};

namespace {

void accept_new_connections(int server_fd, Epoll& epoll) {
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            throw std::runtime_error("accept() failed");
        }

        set_non_blocking(client_fd);
        epoll.add(client_fd);
    }
}

void handle_client_data(int fd, Epoll& epoll) {
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
        if (count == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            epoll.remove(fd);
            close(fd);
            break;
        } else if (count == 0) {
            epoll.remove(fd);
            close(fd);
            break;
        } else {
            send(fd, buffer, count, 0);
        }
    }
}

void run_echo_server() {
    ListeningSocket server(PORT);
    Epoll epoll;
    epoll.add(server);

    while (true) {
        for (const auto& event : epoll.wait()) {
            int fd = event.data.fd;
            if (fd == server) {
                accept_new_connections(server, epoll);
            } else {
                handle_client_data(fd, epoll);
            }
        }
    }
}

} // anonymous namespace

int main() {
    try {
        run_echo_server();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
