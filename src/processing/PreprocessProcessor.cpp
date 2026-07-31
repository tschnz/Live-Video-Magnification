#include "processing/PreprocessProcessor.hpp"

#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>

namespace livim {

FrameRef PreprocessProcessor::process(const FrameRef& in, const ProcessorConfig& cfg) {
    if (in->image.empty()) return in;

    const PreprocessParams& p = cfg.preprocess;
    const int divisor = std::clamp(p.downscale, 1, 8);
    if (!p.roiEnabled && divisor == 1) return in;

    const cv::Mat& src = in->image;

    // ROI is normalized [0,1] against the FULL source frame; clamp inside the frame, keep >= 1px.
    cv::Rect roi(0, 0, src.cols, src.rows);
    if (p.roiEnabled) {
        int x = static_cast<int>(std::lround(static_cast<double>(p.roiX) * src.cols));
        int y = static_cast<int>(std::lround(static_cast<double>(p.roiY) * src.rows));
        int w = static_cast<int>(std::lround(static_cast<double>(p.roiW) * src.cols));
        int h = static_cast<int>(std::lround(static_cast<double>(p.roiH) * src.rows));
        x = std::clamp(x, 0, src.cols - 1);
        y = std::clamp(y, 0, src.rows - 1);
        w = std::clamp(w, 1, src.cols - x);
        h = std::clamp(h, 1, src.rows - y);
        roi = cv::Rect(x, y, w, h);
    }

    // Crop first (header-only view), then resize. Downstream must get its own buffer: the pooled
    // input frame is recycled once we return, so a plain crop is copied out.
    cv::Mat cropped = src(roi);
    cv::Mat outMat;
    if (divisor > 1) {
        const int dw = std::max(1, cropped.cols / divisor);
        const int dh = std::max(1, cropped.rows / divisor);
        cv::resize(cropped, outMat, cv::Size(dw, dh), 0, 0, cv::INTER_AREA);
    } else {
        cropped.copyTo(outMat);
    }

    // Update the size fields; the renderer keys on Frame::width/height, not the cv::Mat dims.
    auto out = std::make_shared<Frame>(*in);
    out->image = outMat;
    out->width = outMat.cols;
    out->height = outMat.rows;
    return out;
}

} // namespace livim
