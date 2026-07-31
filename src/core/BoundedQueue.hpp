#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

namespace livim {

// Overflow behaviour when full:
//   Block : producer waits for space (lossless backpressure).
//   Drop  : evict the oldest to make room (latest-wins).
enum class OverflowPolicy { Block, Drop };

template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity, OverflowPolicy policy = OverflowPolicy::Block)
        : cap_(capacity), policy_(policy) {}

    // Returns false if the queue was stopped instead of accepting the item.
    [[nodiscard]] bool push(T item) {
        std::unique_lock<std::mutex> lk(m_);
        if (policy_ == OverflowPolicy::Block) {
            notFull_.wait(lk, [&] { return q_.size() < cap_ || stopped_; });
            if (stopped_) return false;
            q_.push_back(std::move(item));
        } else {
            if (stopped_) return false;
            if (q_.size() >= cap_) {
                q_.pop_front();
                drops_.fetch_add(1, std::memory_order_relaxed);
            }
            q_.push_back(std::move(item));
        }
        notEmpty_.notify_one();
        return true;
    }

    // Blocks until an item is available; returns false only when stopped and empty.
    [[nodiscard]] bool pop(T& out) {
        std::unique_lock<std::mutex> lk(m_);
        notEmpty_.wait(lk, [&] { return !q_.empty() || stopped_; });
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop_front();
        notFull_.notify_one();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lg(m_);
            stopped_ = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lg(m_);
        std::deque<T>().swap(q_);
        stopped_ = false;
        drops_.store(0, std::memory_order_relaxed);
    }

    // Call only while no producer/consumer is running.
    void setPolicy(OverflowPolicy p) {
        std::lock_guard<std::mutex> lg(m_);
        policy_ = p;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lg(m_);
        return q_.size();
    }

    std::size_t capacity() const { return cap_; }

    std::uint64_t drops() const { return drops_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex m_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::deque<T> q_;
    std::size_t cap_;
    OverflowPolicy policy_;
    bool stopped_ = false;
    std::atomic<std::uint64_t> drops_{0};
};

} // namespace livim
