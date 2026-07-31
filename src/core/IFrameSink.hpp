#pragma once

#include <opencv2/core.hpp>

namespace livim {

// Sink a source can clone grabbed frames into (e.g. lossless camera recording); lives in core/
// so the source layer never depends on the export layer.
class IFrameSink {
public:
    virtual ~IFrameSink() = default;

    // Must copy `bgr` (the source recycles its buffer right after this returns) and must not
    // block the calling source thread.
    virtual void append(const cv::Mat& bgr) = 0;
};

} // namespace livim
