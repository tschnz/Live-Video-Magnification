#pragma once

#include <cstdint>
#include <string>

#include <opencv2/videoio.hpp>

#include "source/SourceBase.hpp"

namespace livim {

// Decodes a video file, paced at a fixed cadence (target playback FPS), emitting in decode order.
// Assumes constant frame rate: ptsUs is synthesized as frameIndex * frameInterval, so a VFR source
// plays at the wrong speed. Pushes losslessly (BLOCK): a slow consumer just backpressures the decoder.
class FileSource : public SourceBase {
public:
    FileSource(std::string path, FrameQueue* out, FramePool* pool, Instrumentation* instr);

    SourceKind kind() const override { return SourceKind::File; }
    bool open() override;
    bool isOpen() const override { return cap_.isOpened(); }
    void setLoop(bool enabled) override { loop_.store(enabled, std::memory_order_release); }
    double reportedFps() const override { return reportedFps_; }
    std::string openNotice() const override {
        return usedSoftwareDecodeFallback_
                   ? "Software decoding fallback: no hardware decoder for this video."
                   : std::string();
    }

    // Seeks go through CAP_PROP_POS_FRAMES, which is keyframe-approximate for long-GOP codecs,
    // so scrub/trim positions are best-effort.
    bool seekable() const override { return frameCount_ > 0; }
    std::int64_t frameCount() const override { return frameCount_; }
    std::int64_t currentFrame() const override { return currentFrame_.load(std::memory_order_acquire); }
    void seekFrame(std::int64_t frame) override;
    void setInOut(std::int64_t in, std::int64_t out) override;
    bool atEnd() const override { return reachedEnd_.load(std::memory_order_acquire); }

protected:
    void run() override;

private:
    std::int64_t effectiveOut() const; // out-point, or frameCount_ (or "infinite" if unknown)

    // Opens `path` into `cap`. forceSoftware pins the FFmpeg decoder to software-only via the
    // open-time CAP_PROP_HW_ACCELERATION parameter; falls back to a plain open if unhonoured.
    static bool openCapture(cv::VideoCapture& cap, const std::string& path, bool forceSoftware);

    std::string path_;
    cv::VideoCapture cap_;
    bool usedSoftwareDecodeFallback_ = false;
    std::uint64_t seq_ = 0;
    double reportedFps_ = 0.0;
    double frameIntervalUs_ = 0.0; // for the synthesized ptsUs
    std::int64_t frameCount_ = 0;  // 0 = unknown
    std::int64_t pos_ = 0;         // index of the NEXT frame to read (source-thread only)
    std::atomic<bool> loop_{false};
    std::atomic<std::int64_t> currentFrame_{0};
    std::atomic<std::int64_t> pendingSeekFrame_{-1}; // -1 = no seek requested
    std::atomic<std::int64_t> inFrame_{0};        // inclusive
    std::atomic<std::int64_t> outFrame_{-1};      // exclusive; -1 = to the end
    std::atomic<bool> reachedEnd_{false};         // parked at the out-point / EOF (non-looping)
};

} // namespace livim
