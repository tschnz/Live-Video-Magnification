#pragma once

#include <string>

#include <opencv2/videoio.hpp>

#include "export/IExportFrameSource.hpp"

namespace livim {

// Re-decodes a video file (or a [start, end) sub-range) for export. Opens its OWN cv::VideoCapture,
// independent of the live FileSource: a sequential, lossless read with no realtime pacing.
class FileExportFrameSource : public IExportFrameSource {
public:
    // [startFrame, endFrame) trims the range; endFrame < 0 means to the end.
    FileExportFrameSource(std::string path, int startFrame = 0, int endFrame = -1);

    bool     open() override;
    int      frameCount() const override { return frameCount_; }
    cv::Size size() const override { return size_; }
    bool     next(cv::Mat& outBgr) override;
    void     close() override;

private:
    // Reopens with the decoder pinned to software-only (CAP_PROP_HW_ACCELERATION is open-time
    // only); falls back to a plain open on builds that ignore the parameter.
    bool openCaptureSoftware();

    std::string      path_;
    int              startFrame_;
    int              endFrame_;   // exclusive; -1 = to end
    cv::VideoCapture cap_;
    int              frameCount_ = -1; // frames that will be delivered (trimmed)
    int              delivered_ = 0;
    bool             softwareFallbackTried_ = false;
    cv::Size         size_{0, 0};
};

} // namespace livim
