#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <vector>
#include <iostream>

class ThreadPool {
public:
    // explicit: 防止 ThreadPool pool = 10; 这种隐式转换
    explicit ThreadPool(size_t threadCount = 8) : isClosed_(false) {
        for(size_t i = 0; i < threadCount; i++) {
            // [this]: Lambda 捕获当前对象指针，以便访问 private 成员
            workers_.emplace_back([this]() {
                while(true) {
                    std::function<void()> task;
                    {
                        // unique_lock: 支持 wait 过程中的解锁和重新加锁
                        std::unique_lock<std::mutex> locker(mtx_);
                        
                        // 等待条件：要么关门了，要么有任务了
                        // 这里的 Lambda 防止虚假唤醒
                        cond_.wait(locker, [this]{ 
                            return isClosed_ || !tasks_.empty(); 
                        });

                        // 收到停止信号且任务队列为空，线程退出
                        if(isClosed_ && tasks_.empty()) {
                            return;
                        }

                        // 取出任务 (std::move 减少拷贝)
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    } 
                    // 出了作用域，unique_lock 自动解锁
                    // 必须在解锁后执行任务，否则所有任务串行化，线程池失去意义
                    task(); 
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> locker(mtx_);
            isClosed_ = true;
        }
        // 唤醒所有线程，让它们检查 isClosed_ 标志并退出
        cond_.notify_all();

        for(std::thread& worker : workers_) {
            if(worker.joinable()) {
                worker.join(); // 等待线程真正结束
            }
        }
    }

    template<class F, class... Args>
    void AddTask(F&& f, Args&&... args) {
        // 1. 完美转发参数，并绑定成无参函数对象
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        
        {
            std::lock_guard<std::mutex> locker(mtx_);
            tasks_.emplace(task);
        }
        // 2. 唤醒一个空闲线程去处理
        cond_.notify_one();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex mtx_;
    std::condition_variable cond_;
    bool isClosed_;
};

#endif // THREADPOOL_H