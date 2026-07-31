#pragma once

#include <atomic>
#include <memory>
#include <utility>

#include "core/AtomicSharedPtr.hpp"
#include "core/Frame.hpp"

namespace livim {

// Processed frame paired with its matching pre-magnification frame, published as ONE object
// so the side-by-side panes always show the exact same frame.
struct DisplayFrame {
    FrameRef processed; // chain output (right pane / single view)
    FrameRef original;  // pre-magnification frame at the same geometry (left pane); may be null
};

using DisplayFrameRef = std::shared_ptr<const DisplayFrame>;

// Latest-wins handoff from processing to display: the ONLY place a frame may be skipped
// (display does not feed the temporal algorithm, so overwriting never affects correctness).
class LatestFrameMailbox {
public:
    void publish(DisplayFrameRef f) { slot_.store(std::move(f), std::memory_order_release); }
    DisplayFrameRef latest() const { return slot_.load(std::memory_order_acquire); }
    void clear() { slot_.store(nullptr, std::memory_order_release); }

private:
    AtomicSharedPtr<const DisplayFrame> slot_{nullptr};
};

} // namespace livim
