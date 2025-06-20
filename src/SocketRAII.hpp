#pragma once

class SocketRAII {
    int fd;

  public:
    explicit SocketRAII(int fd);
    ~SocketRAII();

    SocketRAII(const SocketRAII &) = delete;
    SocketRAII &operator=(const SocketRAII &) = delete;

    operator int() const;
    int release();
};
