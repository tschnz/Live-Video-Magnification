#pragma once

#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>

#include <opencv2/core.hpp>

#include "core/IFrameSink.hpp"

namespace livim {

// Thread-safe, append-only buffer of cloned raw camera frames for lossless capture. The camera
// thread appends; the GUI reads the lock-free counters.
// CAPACITY CAP: raw 1080p30 grows at ~11 GB/min, so an unbounded buffer would OOM-kill the process.
// `maxBytes` (0 = unlimited) bounds it: at the cap the buffer self-closes and raises limitReached(),
// which the GUI turns into a clean automatic Stop-Recording.
class RecordingBuffer : public IFrameSink {
public:
    explicit RecordingBuffer(std::size_t maxBytes = 0) : maxBytes_(maxBytes) {}

    // No-op once closed.
    void append(const cv::Mat& bgr) override;

    // Reject further appends. Call BEFORE quiescing the producer so a late grab can't sneak in.
    void close() { closed_.store(true, std::memory_order_release); }

    std::size_t frameCount() const { return count_.load(std::memory_order_acquire); }
    std::size_t byteCount() const { return bytes_.load(std::memory_order_acquire); }
    std::size_t capacityBytes() const { return maxBytes_; } // 0 = unlimited
    bool limitReached() const { return limitHit_.load(std::memory_order_acquire); }

    // Move the captured frames out. Call only after the producer is quiesced.
    std::deque<cv::Mat> takeFrames();

private:
    const std::size_t        maxBytes_;
    mutable std::mutex       m_;
    std::deque<cv::Mat>      frames_;
    std::atomic<bool>        closed_{false};
    std::atomic<bool>        limitHit_{false};
    std::atomic<std::size_t> count_{0};
    std::atomic<std::size_t> bytes_{0};
};

} // namespace livim
