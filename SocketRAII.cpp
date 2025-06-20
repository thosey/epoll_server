#include "SocketRAII.hpp"
#include <unistd.h>

SocketRAII::SocketRAII(int fd) : fd(fd) {}

SocketRAII::~SocketRAII() {
    if (fd != -1) close(fd);
}

SocketRAII::operator int() const {
    return fd;
}

int SocketRAII::release() {
    int temp = fd;
    fd = -1;
    return temp;
}
