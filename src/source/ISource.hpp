#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <opencv2/core/types.hpp>

namespace livim {

class IFrameSink;
class LatestFrameMailbox;

enum class SourceKind { File, Camera };

// A frame producer. Concrete sources run their own thread and push losslessly (BLOCK) into the
// source->processing queue. Lifecycle: open() -> start() -> play()/pause() -> stop().
class ISource {
public:
    virtual ~ISource() = default;

    virtual SourceKind kind() const = 0;

    virtual bool open() = 0;
    virtual bool isOpen() const = 0;

    // Optional human-readable note from the most recent successful open() (e.g. a decoding
    // fallback); empty when the source opened normally.
    virtual std::string openNotice() const { return {}; }

    // Spawns the grab thread; it starts paused (call play() to begin producing).
    virtual void start() = 0;

    // Request stop, unblock any wait, and join the thread.
    virtual void stop() = 0;

    virtual void play() = 0;
    virtual void pause() = 0;

    // Only meaningful for finite sources (files); no-op for a live camera.
    virtual void setLoop(bool /*enabled*/) {}

    // Native frame rate in Hz, read once at open(); 0 = unknown. Independent of playback cadence.
    virtual double reportedFps() const { return 0.0; }

    // 1 = grayscale, 3 = BGR; 0 until the first frame is grabbed.
    virtual int nativeChannels() const { return 0; }

    // 0x0 until the first frame is grabbed.
    virtual cv::Size nativeSize() const { return cv::Size(0, 0); }

    // Sources never emit faster than this.
    virtual void setPlaybackFps(double /*fps*/) {}

    // Lossless record mode (camera): clone each grabbed frame into `sink` and publish a raw preview to
    // `preview`, bypassing the queue/chain so the grab loop never backpressures.
    virtual void setRecordTarget(std::shared_ptr<IFrameSink> /*sink*/,
                                 LatestFrameMailbox* /*preview*/) {}
    virtual void clearRecordTarget() {}

    // Timeline support (frame domain). Defaults model a non-seekable live source.
    virtual bool seekable() const { return false; }
    virtual std::int64_t frameCount() const { return 0; }   // 0 = unknown / live
    virtual std::int64_t currentFrame() const { return 0; } // index of the latest emitted frame
    virtual void seekFrame(std::int64_t /*frame*/) {}
    // Bound forward playback to [in, out) (out exclusive; out < 0 = to the end). Seeking stays free.
    virtual void setInOut(std::int64_t /*in*/, std::int64_t /*out*/) {}

    // True when a finite source has played to its end/out-point and is parked there (still seekable).
    virtual bool atEnd() const { return false; }

    // True once the grab loop ended on its own (end-of-file) rather than via stop().
    virtual bool finished() const = 0;
};

} // namespace livim
