//
// Created by mauro on 5/16/19.
//

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class Queue {
public:

    T pop() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        auto item = queue_.front();
        queue_.pop();
        return item;
    }

    void pop(T &item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void push(const T &item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

    void push(T &&item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(std::move(item));
        mlock.unlock();
        cond_.notify_one();
    }

    bool empty() {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.empty();
    }

    unsigned long size() {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};