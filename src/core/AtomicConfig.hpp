#pragma once

#include <atomic>
#include <memory>
#include <utility>

#include "core/AtomicSharedPtr.hpp"

namespace livim {

// RCU-style live config: the GUI publishes an immutable config atomically; the processing
// loop reads the current pointer once per frame. No lock is held across both threads.
template <class T>
class AtomicConfig {
public:
    AtomicConfig() : cur_(std::make_shared<const T>()) {}
    explicit AtomicConfig(T initial) : cur_(std::make_shared<const T>(std::move(initial))) {}

    void publish(T value) {
        cur_.store(std::make_shared<const T>(std::move(value)), std::memory_order_release);
    }

    [[nodiscard]] std::shared_ptr<const T> read() const {
        return cur_.load(std::memory_order_acquire);
    }

private:
    // libc++ has no std::atomic<std::shared_ptr> (see core/AtomicSharedPtr.hpp).
    AtomicSharedPtr<const T> cur_;
};

} // namespace livim
