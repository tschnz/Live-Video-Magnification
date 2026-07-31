#include "export/BufferExportFrameSource.hpp"

#include <utility>

namespace livim {

BufferExportFrameSource::BufferExportFrameSource(std::deque<cv::Mat> frames)
    : frames_(std::move(frames)) {}

bool BufferExportFrameSource::open() {
    if (frames_.empty()) return false;
    size_ = frames_.front().size();
    index_ = 0;
    return true;
}

bool BufferExportFrameSource::next(cv::Mat& outBgr) {
    if (index_ >= frames_.size()) return false;
    // Free the already-processed previous frame so a multi-GB capture drains as it is encoded
    // instead of being held until close().
    if (index_ > 0) frames_[index_ - 1] = cv::Mat();
    outBgr = frames_[index_]; // shared, not copied; downstream only reads it
    ++index_;
    return true;
}

void BufferExportFrameSource::close() {
    frames_.clear();
    index_ = 0;
}

} // namespace livim
