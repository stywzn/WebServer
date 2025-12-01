#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <future>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount = 16) : isClosed_(false)
    { // explicit 是干什么的，这个：  是什么，是检查是否到线程池的尾部吗？
        for (int i = 0; i < threadCount; i++)
        {
            // 这个this 访问 private 不是违反了 cpp类封装的特性吗？
            workers_.emplace_back([this]()
                                  {
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> locker(mtx_);
                        cond_.wait(locker,[this]{
                            return isClosed_ || !tasks_.empty();
                        });

                        if(isClosed_ && tasks_.empty()){
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                } });
        }
    }

    // 析构函数：释放所有线程
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> locker(mtx_); // lock_fuard 是用来干什么的，然后notify_all _one 是自带的函数吗？
            isClosed_ = true;
        }
        cond_.notify_all();

        for(std::thread& worker : workers_){
            if(worker.joinable()){
                worker.join();
            }
        }
    }

    // 添加任务的接口
    // 这里用了模板，因为我们不知道用户会传什么函数进来
    // F: 函数类型, Args: 参数包
    template <class F, class... Args>
    void AddTask(F &&f, Args &&...args);

private:
    using Task = std::function<void()>; // why turn any function to void
    std::vector<std::thread> workers_;
    std::queue<Task> tasks_;
    std::mutex mtx_;
    std::condition_variable cond_;
    bool isClosed_;
};

// 模板函数的实现通常要写在头文件里，或者 .tpp 文件里
// 咱们直接写在头文件最下面
// what the meaning of awake a asleep thread   condition.notify_one())
template <class F, class... Args>
void ThreadPool::AddTask(F &&f, Args &&...args)
{
    auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);  //这个是什么意思啊？ arg...
    {
        std::lock_guard<std::mutex> locker(mtx_);
        tasks_.emplace(task);
    }
    cond_.notify_one();
}

#endif // THREADPOOL_H