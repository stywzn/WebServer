#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h> // sockaddr_in
#include <unistd.h>    // close
#include <sys/socket.h>
#include <fcntl.h>     // fcntl
#include <stdexcept>
#include <cassert>

class Socket {
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    void Bind(int port);
    void Listen();
    int Accept(struct sockaddr_in* addr);

    int Fd() const;
    void SetNonBlocking(); // 【面试核心】设置非阻塞
    void SetReuseAddr();   // 【面试核心】设置端口复用

private:
    int fd_;
};

#endif // SOCKET_H