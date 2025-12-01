# High Performance C++ WebServer

这是我基于 Linux C++ 实现的高性能 Web 服务器。

## 核心功能
* **网络模型**：基于 `Epoll` 的 I/O 多路复用，采用 `Reactor` 模式 + 非阻塞 I/O。
* **并发模型**：实现了一个半同步/半反应堆模式的**线程池**，避免线程频繁创建销毁的开销。
* **日志系统**：实现了**异步日志**系统，支持分级、滚动记录，由单独线程负责磁盘 I/O。
* **定时器**：基于**小根堆**实现的定时器，用于断开超时非活动连接。

## 环境要求
* Linux
* C++14
* CMake

## 编译与运行
```bash
mkdir build
cd build
cmake ..
make
./server
