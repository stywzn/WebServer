#include "../include/log.h"
#include <iostream>
#include <unistd.h>

// 这里应该包含你的线程池头文件，比如:
// #include "../include/threadpool.h"

int main() {
    // 1. 初始化日志
    Log::get_instance()->init("ThreadPoolLog", 0, 2000, 800000, 0);
    
    LOG_INFO("========== ThreadPool Test Start ==========");

    // TODO: 在这里编写测试你的线程池的代码
    // 例如：
    // ThreadPool<int> *pool = new ThreadPool<int>();
    // pool->append(new int(1));

    std::cout << "Test threadpool running..." << std::endl;
    
    LOG_INFO("========== ThreadPool Test End ==========");
    
    return 0; // 这是必须的！
}