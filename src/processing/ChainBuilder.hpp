#pragma once

#include <memory>
#include <vector>

#include "core/Frame.hpp"
#include "processing/IProcessor.hpp"

namespace livim {

// Assembles and runs the magnification processing chain; shared by the live pipeline and the
// offline Exporter so the two cannot drift.

// Build the ordered processor list. Preprocess is always first; its output is the
// pre-magnification "original" tap (see runChainOnce).
std::vector<std::unique_ptr<IProcessor>> buildProcessors();

// Run one frame through `chain` and return the final frame. `original` is set to the FIRST stage's
// output (the pre-magnification tap); if the chain is empty, `original` == `in`.
FrameRef runChainOnce(const std::vector<std::unique_ptr<IProcessor>>& chain, const FrameRef& in,
                      const ProcessorConfig& cfg, FrameRef& original);

} // namespace livim
