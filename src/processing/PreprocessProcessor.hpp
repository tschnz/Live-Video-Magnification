#pragma once

#include "processing/IProcessor.hpp"

namespace livim {

// Stateless front-of-chain stage: optional normalized ROI crop followed by an optional 1/2..1/8
// downscale. Identity when there is no ROI and downscale == 1.
class PreprocessProcessor : public IProcessor {
public:
    FrameRef process(const FrameRef& in, const ProcessorConfig& cfg) override;
};

} // namespace livim
