#pragma once

#include "core/BoundedQueue.hpp"
#include "core/Frame.hpp"

namespace livim {

// Source -> processing handoff. Policy is chosen per source in PlaybackController::buildAndStart:
// file = Block (lossless, required by the temporal algorithm), camera = Drop (oldest evicted).
// Frames stay ordered either way -- we skip, never reorder.
using FrameQueue = BoundedQueue<FrameRef>;

} // namespace livim
