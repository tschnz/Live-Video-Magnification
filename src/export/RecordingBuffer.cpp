#include "export/RecordingBuffer.hpp"

#include <utility>

namespace livim {

void RecordingBuffer::append(const cv::Mat& bgr) {
    if (closed_.load(std::memory_order_acquire) || bgr.empty()) return;
    cv::Mat copy = bgr.clone(); // own the pixels so the pooled source frame recycles immediately
    const std::size_t sz = static_cast<std::size_t>(copy.total() * copy.elemSize());
    std::lock_guard<std::mutex> lg(m_);
    if (closed_.load(std::memory_order_acquire)) return; // re-check under the lock (stop handoff race)
    const std::size_t newBytes = bytes_.load(std::memory_order_relaxed) + sz;
    frames_.push_back(std::move(copy));
    bytes_.store(newBytes, std::memory_order_relaxed); // only append() writes bytes_, under the lock
    count_.store(frames_.size(), std::memory_order_release);

    // Self-close at the cap so the GUI can stop recording cleanly instead of OOM-ing.
    if (maxBytes_ > 0 && newBytes >= maxBytes_) {
        closed_.store(true, std::memory_order_release);
        limitHit_.store(true, std::memory_order_release);
    }
}

std::deque<cv::Mat> RecordingBuffer::takeFrames() {
    std::lock_guard<std::mutex> lg(m_);
    std::deque<cv::Mat> out;
    out.swap(frames_);
    count_.store(0, std::memory_order_release);
    return out;
}

} // namespace livim
