#include "processing/GrayscaleProcessor.hpp"

#include <opencv2/imgproc.hpp>

namespace livim {

FrameRef GrayscaleProcessor::process(const FrameRef& in, const ProcessorConfig& cfg) {
    if (!cfg.grayscale) return in;
    if (in->image.channels() == 1) return in;

    // Fresh Frame: never write into the pooled input buffer (frames are treated as immutable).
    auto out = std::make_shared<Frame>(*in);
    cv::cvtColor(in->image, out->image, cv::COLOR_BGR2GRAY);
    out->format = PixelFormat::Gray8;
    return out;
}

} // namespace livim
