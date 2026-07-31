#pragma once

#include <opencv2/core.hpp>

namespace livim {

// An ordered, finite sequence of raw frames for the offline Exporter: a video file or an in-memory
// camera recording. Used from the export worker thread only -- not thread-safe.
class IExportFrameSource {
public:
    virtual ~IExportFrameSource() = default;

    virtual bool open() = 0;

    // Total frames to deliver (trimmed count for a ranged file), or -1 = unknown (indeterminate
    // progress).
    virtual int frameCount() const = 0;

    // Raw frame dimensions (before any preprocessing).
    virtual cv::Size size() const = 0;

    // Fetch the next frame (BGR8, or single channel for a mono source), strictly in order.
    // Returns false at end of stream.
    virtual bool next(cv::Mat& outBgr) = 0;

    virtual void close() = 0;
};

} // namespace livim
