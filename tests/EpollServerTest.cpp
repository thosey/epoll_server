#include <gtest/gtest.h>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../src/EpollServer.hpp"

TEST(EpollServerTest, AcceptsConnectionAndEchoesData) {
    EpollServer server(8);

    // Run server in a background thread
    std::thread server_thread([&]() {
        // Only process a few events for the test
        server.processEvents(EpollServer::Once);
        server.processEvents(EpollServer::Once);
    });

    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect as a client
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_fd, -1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ASSERT_EQ(connect(client_fd, (sockaddr*)&addr, sizeof(addr)), 0);

    // Send data
    const char msg[] = "hello";
    ASSERT_EQ(send(client_fd, msg, sizeof(msg), 0), sizeof(msg));

    // Receive echo
    char buf[16] = {};
    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
    ASSERT_GT(n, 0);
    ASSERT_EQ(std::string(buf, n), std::string(msg, sizeof(msg)));

    close(client_fd);
    server_thread.join();
}