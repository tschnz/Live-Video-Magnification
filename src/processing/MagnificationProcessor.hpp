#pragma once

#include <opencv2/core.hpp>

#include "processing/IProcessor.hpp"
#include "processing/magnification/MagnifyCore.hpp"

namespace livim {

// CPU Eulerian video magnification stage wrapping magnification/MagnifyCore.hpp. Owns the per-mode
// temporal state and self-resets on structural changes (mode/levels/size/channels/preprocess
// geometry). Identity when mode == None.
class MagnificationProcessor : public IProcessor {
public:
    FrameRef process(const FrameRef& in, const ProcessorConfig& cfg) override;
    void reset() override;

private:
    magcore::StructuralTracker tracker_;
    magcore::MotionState       motion_;
    magcore::ColorState        color_;
    magcore::RieszState        riesz_;
};

} // namespace livim
