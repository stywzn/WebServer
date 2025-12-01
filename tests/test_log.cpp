// tests/test_log.cpp
#include "../include/log.h" // 注意路径，因为在 tests 目录下
#include <unistd.h> // for sleep

int main() {
    // 1. 初始化日志
    // 异步模式(max_queue_size>0)，写到 "ServerLog" 文件
    Log::get_instance()->init("ServerLog", 0, 2000, 800000, 800);
    
    // 2. 打印不同级别的日志
    LOG_INFO("========== Server Start ==========");
    LOG_INFO("User %s login", "admin");
    LOG_WARN("Disk usage at %d%%", 85);
    LOG_ERROR("System error: %s", "Network Timeout");

    // 3. 模拟一些耗时操作
    for(int i = 0; i < 5; i++) {
        LOG_DEBUG("Current count: %d", i);
    }

    LOG_INFO("========== Server End ==========");

    // 等待异步线程写完（重要！否则主线程结束了，后台写线程可能还没写完）
    sleep(1); 
    return 0;
}