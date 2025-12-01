#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template <class T>
class BlockQueue
{
public:
    // 初始化列表：初始化互斥锁等资源
    explicit BlockQueue(size_t max_capacity = 1000)
    {
        if (max_capacity <= 0)
        {
            exit(-1);
        }
        m_max_capacity = max_capacity;
    }

    // 析构函数：清理资源
    ~BlockQueue()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_queue.clear();
    }

    // 清空队列
    void clear()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_queue.clear();
    }

    // 判断队列是否满
    bool full()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size() >= m_max_capacity;
    }

    // 判断队列是否空
    bool empty()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.empty();
    }

    // 返回队首元素
    bool front(T &value)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_queue.empty())
            return false;
        value = m_queue.front();
        return true;
    }

    // 返回队尾元素
    bool back(T &value)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_queue.empty())
            return false;
        value = m_queue.back();
        return true;
    }

    // 生产者：往队列里塞东西
    bool push(const T &item)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        
        // 1. 如果满了，等待【生产者条件变量】
        while (m_queue.size() >= m_max_capacity)
        {
            // 修正点：这里要等 producer 信号，因为是消费者 pop 之后发 producer 信号
            m_cond_producer.wait(locker);
        }

        m_queue.push_back(item);
        
        // 2. 入队成功，通知【消费者】：有数据了，快来拿
        m_cond_consumer.notify_one();
        return true;
    }

    // 消费者：从队列里取东西
    bool pop(T &item)
    {
        std::unique_lock<std::mutex> locker(m_mutex);

        // 1. 如果空了，等待【消费者条件变量】
        while (m_queue.empty())
        {
            // 这里没问题，等 consumer 信号
            m_cond_consumer.wait(locker);
        }

        item = m_queue.front();
        m_queue.pop_front();

        // 2. 出队成功，通知【生产者】：有空位了，你可以继续塞了
        m_cond_producer.notify_one(); 
        
        return true;
    }

private:
    std::deque<T> m_queue;                   
    size_t m_max_capacity;                   
    std::mutex m_mutex;                      
    std::condition_variable m_cond_consumer; // 用于通知消费者 (队列不空了)
    std::condition_variable m_cond_producer; // 用于通知生产者 (队列不满了)
};

#endif