#include "Socket.h"
#include "Epoller.h"
#include "Buffer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "ThreadPool.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include "../include/log.h"
#include "../include/heaptimer.h"
#include <functional>

// --- 全局变量 ---
HeapTimer timer_manager;
int epollfd = 0; // 全局 epollfd，供 CloseConn 使用

// --- 回调函数：超时关闭连接 ---
void CloseConn(int fd)
{
    // 1. 从 epoll 中删除
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    // 2. 关闭 socket
    close(fd);
    LOG_INFO("Client[%d] timeout, closed!", fd);
}

// 设置非阻塞
void setNonBlocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, opts | O_NONBLOCK);
}

// --- 业务逻辑 (子线程运行) ---
void process(int fd, std::string srcDir, Epoller *epoller)
{
    Buffer buff;
    int saveErrno = 0;

    // 1. 读取数据 (ET模式一次性读完)
    ssize_t len = buff.readFd(fd, &saveErrno);

    if (len > 0)
    {
        // 2. 解析 HTTP 请求
        HttpRequest request;
        if (request.parse(buff))
        {
            HttpResponse response;
            std::string path = request.path();
            response.Init(srcDir, path, false, 200);

            Buffer writeBuff;
            response.MakeResponse(writeBuff);

            struct iovec iov[2];
            iov[0].iov_base = const_cast<char *>(writeBuff.peek());
            iov[0].iov_len = writeBuff.readableBytes();

            if (response.File() && response.FileLen() > 0)
            {
                iov[1].iov_base = response.File();
                iov[1].iov_len = response.FileLen();
            }
            else
            {
                iov[1].iov_base = nullptr;
                iov[1].iov_len = 0;
            }
            // 发送响应
            writev(fd, iov, 2);
        }
    }
    else if (len == 0)
    {
        CloseConn(fd); // 对端关闭
        return;
    }
    else
    {
        if (saveErrno != EAGAIN && saveErrno != EWOULDBLOCK)
        {
            CloseConn(fd); // 出错关闭
            return;
        }
    }

    // 重置 ONESHOT，让主线程能再次检测到该 fd
    epoller->ModFd(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
}

int main()
{
    // 1. 初始化日志
    // 异步日志，队列容量1024
    Log::get_instance()->init("ServerLog", 0, 2000, 800000, 1024);
    
    int port = 8080;
    char cwd[256];
    getcwd(cwd, 256);
    std::string srcDir = std::string(cwd) + "/resources";

    // 2. 初始化线程池
    ThreadPool threadpool(6);

    // 3. 初始化 Socket
    Socket servSock;
    servSock.SetReuseAddr();
    servSock.Bind(port);
    servSock.Listen();
    servSock.SetNonBlocking();

    // 4. 初始化 Epoll
    Epoller epoller;
    epollfd = epoller.GetFd(); // 【注意】确保你的 Epoller 类有 GetFd() 方法，如果没有，请看下面注释
    // 如果 Epoller 没有 GetFd()，你需要去 Epoller.h 加一行: int GetFd() { return epollFd_; }

    epoller.AddFd(servSock.Fd(), EPOLLIN | EPOLLET);

    LOG_INFO(">> Server running on http://localhost:%d", port);
    std::cout << ">> Server running on http://localhost:" << port << std::endl;

    // 定义 epoll_wait 需要的数组
    const int MAX_EVENT_NUMBER = 10000;
    struct epoll_event events[MAX_EVENT_NUMBER];

    while (true)
    {
        // 5. 获取最近的超时时间
        int time_ms = timer_manager.getNextTick();
        
        // 阻塞等待事件
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, time_ms);

        if (number < 0 && errno != EINTR) {
            LOG_ERROR("Epoll wait failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            // A. 处理新连接
            if (sockfd == servSock.Fd())
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(servSock.Fd(), (struct sockaddr *)&client_address, &client_addrlength);
                
                if (connfd < 0) continue;

                setNonBlocking(connfd);
                
                // 添加到 epoll 监控
                epoller.AddFd(connfd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                
                // 【新增】添加定时器 (60秒)
                timer_manager.add(connfd, 60000, std::bind(CloseConn, connfd));
                LOG_INFO("New client[%d] connected", connfd);
            }
            
            // B. 处理异常断开
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                timer_manager.doWork(sockfd); // 移除定时器并关闭
            }
            
            // C. 处理读事件
            else if (events[i].events & EPOLLIN)
            {
                // 【重要】主线程只负责“续命”和“分发”
                // 1. 只要有活动，就更新定时器 (往后延60秒)
                timer_manager.add(sockfd, 60000, std::bind(CloseConn, sockfd));
                
                // 2. 交给线程池去处理具体的读写业务
                threadpool.AddTask(std::bind(process, sockfd, srcDir, &epoller));
            }
            
            // D. 处理写事件
            else if (events[i].events & EPOLLOUT)
            {
                 // 也是续命 + 分发（这里简单起见，写逻辑通常也在 process 里或者单独函数）
                 timer_manager.add(sockfd, 60000, std::bind(CloseConn, sockfd));
                 // 这里暂不处理写事件的分发，通常 WebServer 是读完直接写的
            }
        }
        
        // 6. 处理超时连接
        timer_manager.tick();
    }
    return 0;
}