#include "export/FileExportFrameSource.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace livim {

FileExportFrameSource::FileExportFrameSource(std::string path, int startFrame, int endFrame)
    : path_(std::move(path)), startFrame_(std::max(0, startFrame)), endFrame_(endFrame) {}

bool FileExportFrameSource::openCaptureSoftware() {
    if (cap_.open(cv::String(path_), static_cast<int>(cv::CAP_FFMPEG),
                  std::vector<int>{cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_NONE}))
        return true;
    cap_.release();
    return cap_.open(path_);
}

bool FileExportFrameSource::open() {
    if (!cap_.open(path_)) return false;

    const double total = cap_.get(cv::CAP_PROP_FRAME_COUNT);
    const int totalFrames = total > 0.0 ? static_cast<int>(total) : -1;

    // Resolve the trimmed range; unknown total -> deliver to natural EOF with an unknown count.
    int end = endFrame_;
    if (totalFrames > 0) {
        if (end < 0 || end > totalFrames) end = totalFrames;
        const int start = std::min(startFrame_, totalFrames);
        frameCount_ = std::max(0, end - start);
    } else {
        frameCount_ = -1; // unknown -> indeterminate progress
    }
    endFrame_ = end;

    if (startFrame_ > 0) cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(startFrame_));

    int w = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
    int h = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (w <= 0 || h <= 0) {
        cv::Mat probe;
        if (!cap_.read(probe)) return false;
        w = probe.cols;
        h = probe.rows;
        cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(startFrame_)); // rewind to the in-point
    }
    size_ = cv::Size(w, h);
    delivered_ = 0;
    return true;
}

bool FileExportFrameSource::next(cv::Mat& outBgr) {
    if (endFrame_ >= 0 && startFrame_ + delivered_ >= endFrame_) return false; // out-point
    if (cap_.read(outBgr)) {
        ++delivered_;
        return true;
    }
    // Decode failure — typically a hardware-accelerated codec (e.g. AV1) with no hardware decoder,
    // where the FFmpeg backend prints "Failed to get pixel format" / "Get current frame error".
    // Reopen once pinned to pure software and resume from the in-point + frames already delivered.
    if (softwareFallbackTried_) return false;
    softwareFallbackTried_ = true;
    cap_.release();
    if (!openCaptureSoftware()) return false;
    cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(startFrame_ + delivered_));
    if (!cap_.read(outBgr)) return false;
    ++delivered_;
    return true;
}

void FileExportFrameSource::close() {
    if (cap_.isOpened()) cap_.release();
}

} // namespace livim
