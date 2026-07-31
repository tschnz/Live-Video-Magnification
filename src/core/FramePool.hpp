#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "core/Frame.hpp"

namespace livim {

// Fixed-size pool of reusable Frames; the MutableFrameRef deleter returns buffers to the
// pool, so cv::Mat storage is reused. On exhaustion acquire() BLOCKS (backpressure).
// In-flight frames keep the shared Core alive, so storage outlives any outstanding frame
// even if the FramePool is destroyed.
class FramePool {
public:
    explicit FramePool(std::size_t capacity);

    FramePool(const FramePool&) = delete;
    FramePool& operator=(const FramePool&) = delete;
    FramePool(FramePool&&) = delete;
    FramePool& operator=(FramePool&&) = delete;

    // Blocks until a buffer is free. Returns nullptr if the pool was stopped.
    [[nodiscard]] MutableFrameRef acquire();

    // Unblocks waiters, which then get nullptr from acquire().
    void stop();

    void reset();

    std::size_t capacity() const { return capacity_; }

private:
    struct Core {
        std::mutex m;
        std::condition_variable cv;
        std::vector<std::unique_ptr<Frame>> storage;
        std::vector<Frame*> freeList;
        bool stopped = false;
    };

    std::shared_ptr<Core> core_;
    std::size_t capacity_;
};

} // namespace livim
