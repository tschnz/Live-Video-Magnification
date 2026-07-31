#pragma once

#include <cstddef>
#include <deque>

#include <opencv2/core.hpp>

#include "export/IExportFrameSource.hpp"

namespace livim {

// Replays an in-memory deque of raw camera frames (captured by RecordingBuffer) for export.
// Owns the frames (moved in).
class BufferExportFrameSource : public IExportFrameSource {
public:
    explicit BufferExportFrameSource(std::deque<cv::Mat> frames);

    bool     open() override;
    int      frameCount() const override { return static_cast<int>(frames_.size()); }
    cv::Size size() const override { return size_; }
    bool     next(cv::Mat& outBgr) override;
    void     close() override;

private:
    std::deque<cv::Mat> frames_;
    std::size_t         index_ = 0;
    cv::Size            size_{0, 0};
};

} // namespace livim
