#pragma once
#include <condition_variable>
#include <mutex>
#include <deque>
#include <optional>

template<typename T>
class ThreadSafeQueue {
public:
    void push(T v) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push_back(std::move(v));
        }
        cv_.notify_one();
    }

    // Blocking pop
    T pop_wait() {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&]{ return !q_.empty(); });
        T v = std::move(q_.front());
        q_.pop_front();
        return v;
    }

    // Non-blocking pop
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop_front();
        return v;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(m_);
        return q_.empty();
    }

private:
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::deque<T> q_;
};
