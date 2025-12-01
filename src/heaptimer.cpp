#include "../include/heaptimer.h"

// 交换两个节点，并且更新它们在 map 中的下标映射
void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());

    std::swap(heap_[i], heap_[j]);

    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

// 上滤操作
void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(heap_[j] < heap_[i]) { break; }
        swapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

// 下滤操作
bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) return false;
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

// 添加定时器
void HeapTimer::add(int id, int timeOut, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id)) {
        i = ref_[id];
        heap_[i].expire = Clock::now() + MS(timeOut);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    } else {
        i = heap_.size();
        ref_[id] = i;
        TimerNode node;
        node.id = id;
        node.expire = Clock::now() + MS(timeOut);
        node.cb = cb;
        heap_.push_back(node);
        siftup_(i);
    }
}

// 删除指定连接的定时器
void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    size_t last = heap_.size() - 1;
    if (i != last) {
        swapNode_(i, last);
        if(!siftdown_(i, last)) {
            siftup_(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

// 清除超时节点
void HeapTimer::tick() {
    if(heap_.empty()) { return; }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    size_t last = heap_.size() - 1;
    ref_.erase(heap_.front().id);
    if (0 != last) {
        swapNode_(0, last);
        heap_.pop_back();
        siftdown_(0, heap_.size());
    } else {
        heap_.pop_back();
    }
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expire - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}