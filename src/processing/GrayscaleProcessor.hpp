#pragma once

#include "processing/IProcessor.hpp"

namespace livim {

// Converts a BGR cv::Mat to single-channel Gray8. Identity when grayscale is disabled or the frame
// is already single channel.
class GrayscaleProcessor : public IProcessor {
public:
    FrameRef process(const FrameRef& in, const ProcessorConfig& cfg) override;
};

} // namespace livim
