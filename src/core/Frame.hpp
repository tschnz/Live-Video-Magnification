#pragma once

#include <cstdint>
#include <memory>

#include <opencv2/core.hpp>

#include "core/Clock.hpp"

namespace livim {

enum class PixelFormat { BGR8, Gray8 };

// Frames come from FramePool and are recycled when the last reference drops, so
// transport does no per-frame allocation.
struct Frame {
    std::uint64_t seq = 0;        // monotonic per-source sequence id (gaps == display skips)
    std::int64_t  ptsUs = 0;      // source presentation timestamp, microseconds
    Timestamp     captureTs{};    // grab time, for end-to-end latency
    int           width = 0;
    int           height = 0;
    PixelFormat   format = PixelFormat::BGR8;

    cv::Mat image;
};

// Consumers see frames as immutable. Producers fill a MutableFrameRef, then publish it
// as a FrameRef (shared control block, so the pool's recycling deleter still fires).
using FrameRef = std::shared_ptr<const Frame>;
using MutableFrameRef = std::shared_ptr<Frame>;

} // namespace livim
