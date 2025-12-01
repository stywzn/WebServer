#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <unistd.h>  // for read, write
#include <sys/uio.h> // for readv

class Buffer {
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    size_t writableBytes() const;       // 可写字节数
    size_t readableBytes() const;       // 可读字节数
    size_t prependableBytes() const;    // 头部预留空间

    const char* peek() const;           // 查看当前读指针位置
    void retrieve(size_t len);          // 读走 len 长度数据，指针后移
    void retrieveUntil(const char* end);// 读走直到 end 位置
    void retrieveAll();                 // 清空
    std::string retrieveAllToStr();     // 取出所有数据转 string

    void append(const std::string& str);
    void append(const char* str, size_t len);
    void append(const void* data, size_t len);

    ssize_t readFd(int fd, int* Errno); // 【核心难点】从 socket 读数据
    ssize_t writeFd(int fd, int* Errno);// 往 socket 写数据

private:
    char* BeginPtr_();                  // 获取内存起始指针
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);        // 自动扩容

    std::vector<char> buffer_;          // 使用 vector 管理内存 (RAII)
    std::atomic<size_t> readPos_;       // 读指针 (原子操作保证线程安全基础，虽然后续还要加锁)
    std::atomic<size_t> writePos_;      // 写指针
};

#endif // BUFFER_H