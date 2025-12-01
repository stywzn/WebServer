#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;             // 连接的 socket fd，用来标记是谁的定时器
    TimeStamp expire;   // 过期时间点
    TimeoutCallBack cb; // 回调函数：超时了要执行什么？(比如关闭连接)

    // 需要重载 < 运算符，方便比较谁先过期
    bool operator<(const TimerNode& t) const {
        return expire < t.expire;
    }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); } // 预留一点空间

    ~HeapTimer() { clear(); }
    
    // 调整定时器：如果这个连接有活动，过期时间要往后延
    void adjust(int id, int newExpires);

    // 添加定时器
    void add(int id, int timeOut, const TimeoutCallBack& cb);

    // 删除某个连接的定时器
    void doWork(int id);

    // 核心逻辑：清除所有超时节点
    void tick();

    // 删除堆顶（最早过期的）
    void pop();

    // 获取下一次超时还要多久（传给 epoll_wait 用）
    int getNextTick();

    void clear();

private:
    // 两个核心辅助函数：上滤和下滤 (堆排序核心)
    void siftup_(size_t i);
    bool siftdown_(size_t index, size_t n);

    // 交换节点
    void swapNode_(size_t i, size_t j);

    // 数据存储：用 vector 模拟数组堆
    std::vector<TimerNode> heap_;

    // 关键优化：映射表
    // 为什么需要它？
    // 因为我们要通过 socket fd 快速找到它在 heap_ 数组里的下标，否则查找是 O(N)
    std::unordered_map<int, size_t> ref_; 
};

#endif // HEAP_TIMER_H