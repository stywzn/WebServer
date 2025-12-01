#include "Socket.h"

Socket::Socket() {
    // AF_INET: IPv4, SOCK_STREAM: TCP
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_ < 0) {
        throw std::runtime_error("Create socket error!");
    }
}

Socket::Socket(int fd) : fd_(fd) {
    if(fd_ < 0) {
        throw std::runtime_error("Socket init error!");
    }
}

Socket::~Socket() {
    if(fd_ >= 0) {
        close(fd_); // RAII: 对象销毁时自动关闭 fd，防止资源泄漏
    }
}

void Socket::Bind(int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有网卡
    addr.sin_port = htons(port);

    if(bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Bind error!");
    }
}

void Socket::Listen() {
    // SOMAXCONN: 系统允许的最大的挂起连接数 (backlog)
    if(listen(fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Listen error!");
    }
}

int Socket::Accept(struct sockaddr_in* addr) {
    socklen_t len = sizeof(*addr);
    int clientFd = accept(fd_, (struct sockaddr*)addr, &len);
    // 注意：这里我们不抛异常，因为 accept 失败可能是暂时的，交给上层处理
    return clientFd; 
}

int Socket::Fd() const {
    return fd_;
}

// 【面试必考】设置端口复用
// 场景：服务器重启时，如果之前的连接还处于 TIME_WAIT 状态，
// 没有这个选项会导致 bind 失败，提示 Address already in use。
void Socket::SetReuseAddr() {
    int optval = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

// 【面试必考】设置非阻塞
// 场景：配合 Epoll 的 ET (边缘触发) 模式必须使用非阻塞 IO
void Socket::SetNonBlocking() {
    int opts = fcntl(fd_, F_GETFL);
    if(opts < 0) {
        return;
    }
    opts = opts | O_NONBLOCK;
    fcntl(fd_, F_SETFL, opts);
}