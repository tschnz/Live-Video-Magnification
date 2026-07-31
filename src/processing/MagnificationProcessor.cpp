#include "processing/MagnificationProcessor.hpp"

#include <algorithm>
#include <utility>

#include "processing/magnification/SpatialFilter.hpp" // calculateMaxLevels

namespace livim {

void MagnificationProcessor::reset() {
    motion_.reset();
    color_.reset();
    riesz_.reset();
    tracker_.reset();
}

FrameRef MagnificationProcessor::process(const FrameRef& in, const ProcessorConfig& cfg) {
    const MagnificationParams& p = cfg.magnification;

    // Identity when disabled; free state so a later re-enable starts cleanly.
    if (p.mode == MagnificationMode::None || in->image.empty()) {
        if (tracker_.mode != MagnificationMode::None) {
            motion_.reset();
            color_.reset();
            riesz_.reset();
            tracker_.disable();
        }
        return in;
    }

    // Clamp levels to what this frame size supports; too small to magnify (<=5px) -> identity.
    const int maxLevels = calculateMaxLevels(in->image.size());
    if (maxLevels < 1) return in;
    const int levels = std::clamp(p.levels, 1, maxLevels);
    const int channels = in->image.channels();
    const cv::Size size = in->image.size();

    // Reset temporal state on any structural change (see StructuralTracker).
    if (tracker_.update(cfg, levels, channels, size)) {
        motion_.reset();
        color_.reset();
        riesz_.reset();
    }

    cv::Mat out8u;
    PixelFormat fmt = in->format;
    bool produced = false;
    switch (p.mode) {
    case MagnificationMode::Laplace:
        produced = magcore::magnifyMotion(in->image, p, levels, channels, motion_, out8u, fmt);
        break;
    case MagnificationMode::Color:
        produced = magcore::magnifyColor(in->image, p, levels, channels, color_, out8u, fmt);
        break;
    case MagnificationMode::Phase:
        produced = magcore::magnifyRiesz(in->image, p, levels, channels, riesz_, out8u, fmt);
        break;
    case MagnificationMode::None:
        return in;
    }
    if (!produced) return in; // warmup / unsupported input: emit the input unchanged

    auto out = std::make_shared<Frame>(*in);
    out->image = std::move(out8u); // fresh buffer; never aliases in->image
    out->format = fmt;
    return out;
}

} // namespace livim
