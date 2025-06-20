#include <gtest/gtest.h>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../src/EpollServer.hpp"

TEST(EpollServerTest, AcceptsConnectionAndEchoesData) {
    EpollServer server(8);

    std::thread server_thread([&]() {
        server.processEvents(EpollServer::Once);
        server.processEvents(EpollServer::Once);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_fd, -1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ASSERT_EQ(connect(client_fd, (sockaddr*)&addr, sizeof(addr)), 0);

    const char msg[] = "hello";
    ASSERT_EQ(send(client_fd, msg, sizeof(msg), 0), sizeof(msg));

    char buf[16] = {};
    ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
    ASSERT_GT(n, 0);
    ASSERT_EQ(std::string(buf, n), std::string(msg, sizeof(msg)));

    close(client_fd);
    server_thread.join();
}

TEST(EpollServerTest, BuffersWhenSendWouldBlock) {
    EpollServer server(8);

    bool done = std::atomic<bool>(false);
    std::thread server_thread([&]() {
        while (!done) {
            server.processEvents(EpollServer::Once);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_fd, -1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ASSERT_EQ(connect(client_fd, (sockaddr*)&addr, sizeof(addr)), 0);

    constexpr size_t big_size = 1024 * 1024; // 1MB
    std::string big_msg(big_size, 'A');
    ASSERT_EQ(send(client_fd, big_msg.data(), big_msg.size(), 0), big_msg.size());

    size_t total = 0;
    char buf[4096];
    while (total < big_size) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; ++i) ASSERT_EQ(buf[i], 'A');
        total += n;
    }
    ASSERT_EQ(total, big_size);

    close(client_fd);
    done = true; // Signal the server thread to exit
    server_thread.join();
}