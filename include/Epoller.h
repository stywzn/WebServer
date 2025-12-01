#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> // epoll_ctl()
#include <unistd.h>    // close()
#include <vector>
#include <cassert>

class Epoller
{
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    // 在 class Epoller 的 public 里加上：
    int GetFd() const { return epollFd_; } // 假设你的私有变量叫 epollFd_

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);

    // 等待事件发生，返回事件数量
    int Wait(int timeoutMs = -1);

    // 获取第 i 个事件的 fd
    int GetEventFd(size_t i) const;
    // 获取第 i 个事件的 events (如 EPOLLIN)
    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;
    // 用于存放 epoll_wait 返回的活跃事件
    std::vector<struct epoll_event> events_;
};

#endif // EPOLLER_H