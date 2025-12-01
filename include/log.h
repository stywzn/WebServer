#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

class Log
{
public:
    // C++11懒汉模式，线程安全
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    // 异步写线程回调函数
    static void *flush_log_thread(void *args)
    {
        (void)args;
        Log::get_instance()->async_write_log();
        return nullptr;
    }

    // 初始化：文件名、关闭日志标志、缓冲区大小、最大行数、队列容量
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();

    // 真正的异步写逻辑
    void async_write_log()
    {
        std::string single_log;
        // 从队列取出一条日志，写入文件
        while (m_log_queue->pop(single_log))
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            fputs(single_log.c_str(), m_fp);
        }
    }

private:
    char dir_name[128]; // 路径名
    char log_name[128]; // log文件名
    int m_split_lines;  // 日志最大行数
    int m_log_buf_size; // 日志缓冲区大小
    long long m_count;  // 日志行数记录
    int m_today;        // 记录当前时间
    FILE *m_fp;         // 文件指针
    char *m_buf;        // 缓冲区
    BlockQueue<std::string> *m_log_queue; // 阻塞队列
    bool m_is_async;    // 是否同步标志位
    std::mutex m_mutex;
    int m_close_log;    // 关闭日志标志
};

// 下面是宏定义，写成单行模式，防止格式化工具搞乱
// 0:debug, 1:info, 2:warn, 3:error

#define LOG_DEBUG(format, ...) do {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();} while(0)
#define LOG_INFO(format, ...)  do {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();} while(0)
#define LOG_WARN(format, ...)  do {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();} while(0)
#define LOG_ERROR(format, ...) do {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();} while(0)

#endif