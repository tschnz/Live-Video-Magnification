#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

namespace livim {

// Portable stand-in for C++20 std::atomic<std::shared_ptr<T>> (P0718): libc++ does not
// implement it (LLVM issue #99980), so one mutex-guarded path is used on every stdlib.
template <class T>
class AtomicSharedPtr {
public:
    AtomicSharedPtr() noexcept = default;
    AtomicSharedPtr(std::nullptr_t) noexcept {}
    AtomicSharedPtr(std::shared_ptr<T> p) noexcept : p_(std::move(p)) {}

    AtomicSharedPtr(const AtomicSharedPtr&)            = delete;
    AtomicSharedPtr& operator=(const AtomicSharedPtr&) = delete;

    // memory_order args are accepted for API parity and ignored: the mutex is a stronger
    // barrier than the acquire/release callers request.
    [[nodiscard]] std::shared_ptr<T> load(std::memory_order = std::memory_order_seq_cst) const {
        std::lock_guard<std::mutex> lk(m_);
        return p_;
    }

    void store(std::shared_ptr<T> p, std::memory_order = std::memory_order_seq_cst) {
        std::shared_ptr<T> old; // keeps the previous owner alive so its deleter runs outside the lock
        {
            std::lock_guard<std::mutex> lk(m_);
            old.swap(p_);
            p_ = std::move(p);
        }
    }

private:
    mutable std::mutex m_;
    std::shared_ptr<T> p_;
};

} // namespace livim
