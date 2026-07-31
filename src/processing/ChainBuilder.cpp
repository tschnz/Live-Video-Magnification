#include "processing/ChainBuilder.hpp"

#include <cstddef>

#include "processing/GrayscaleProcessor.hpp"
#include "processing/MagnificationProcessor.hpp"
#include "processing/PreprocessProcessor.hpp"

namespace livim {

std::vector<std::unique_ptr<IProcessor>> buildProcessors() {
    std::vector<std::unique_ptr<IProcessor>> procs;
    procs.push_back(std::make_unique<PreprocessProcessor>()); // also the "original" tap
    procs.push_back(std::make_unique<GrayscaleProcessor>());
    procs.push_back(std::make_unique<MagnificationProcessor>());
    return procs;
}

FrameRef runChainOnce(const std::vector<std::unique_ptr<IProcessor>>& chain, const FrameRef& in,
                      const ProcessorConfig& cfg, FrameRef& original) {
    FrameRef cur = in;
    original = nullptr;
    for (std::size_t i = 0; i < chain.size(); ++i) {
        cur = chain[i]->process(cur, cfg);
        if (i == 0) original = cur; // pre-magnification tap
    }
    if (!original) original = in;
    return cur;
}

} // namespace livim
