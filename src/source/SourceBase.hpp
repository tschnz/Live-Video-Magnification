#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "core/AtomicSharedPtr.hpp"
#include "core/Clock.hpp"
#include "core/Frame.hpp"
#include "core/PipelineTypes.hpp"
#include "source/ISource.hpp"

namespace livim {

class FramePool;
class Instrumentation;
class IFrameSink;
class LatestFrameMailbox;

// While set, a source clones every grabbed frame into `sink` and publishes a raw preview to
// `preview`, bypassing the queue/chain so nothing backpressures the grab loop.
struct RecordTarget {
    std::shared_ptr<IFrameSink> sink;
    LatestFrameMailbox*         preview = nullptr;
};

// Shared thread lifecycle + pause/stop plumbing for sources. Derived classes implement run() as the
// grab loop; the base owns the std::thread. The thread begins paused so Open does not auto-play.
class SourceBase : public ISource {
public:
    SourceBase(FrameQueue* out, FramePool* pool, Instrumentation* instr);
    ~SourceBase() override;

    void start() override;
    void stop() override;
    void play() override;
    void pause() override;

    // GUI thread writes; the source thread reads it once per frame in paceFrame().
    void setPlaybackFps(double fps) override;

    // GUI thread writes; the source thread reads it once per frame.
    void setRecordTarget(std::shared_ptr<IFrameSink> sink, LatestFrameMailbox* preview) override;
    void clearRecordTarget() override;

    bool finished() const override { return finished_.load(std::memory_order_acquire); }

    // Source thread writes the two accessors below; the GUI reads them.
    int nativeChannels() const override { return nativeChannels_.load(std::memory_order_acquire); }

    cv::Size nativeSize() const override {
        return cv::Size(nativeWidth_.load(std::memory_order_acquire),
                        nativeHeight_.load(std::memory_order_acquire));
    }

protected:
    virtual void run() = 0; // derived grab loop

    void setNativeChannels(int channels) {
        nativeChannels_.store(channels, std::memory_order_release);
    }

    void setNativeSize(int width, int height) {
        nativeWidth_.store(width, std::memory_order_release);
        nativeHeight_.store(height, std::memory_order_release);
    }

    bool stopRequested() const { return stop_.load(std::memory_order_acquire); }
    bool paused() const { return paused_.load(std::memory_order_acquire); }

    // nullptr when not recording. Read once per frame by the grab loop.
    std::shared_ptr<RecordTarget> recordTarget() const {
        return recordTarget_.load(std::memory_order_acquire);
    }

    // If paused, block until resumed or stopped; returns the duration spent paused (0 if not).
    // `extraWake` lets a derived source break out early (e.g. FileSource wakes on a pending seek).
    Clock::duration waitWhilePaused(const std::function<bool()>& extraWake = {});

    void wakePauseWaiters();

    // Returns nullptr if the pool/source is stopping.
    MutableFrameRef acquireFrame();

    // Push a frame downstream (BLOCK / lossless). Returns false if stopping.
    bool emit(FrameRef f);

    // resetPacing() re-anchors the cadence clock; call it whenever playback (re)starts. paceFrame()
    // blocks until this frame's scheduled emit time; if behind, it drops the deficit (never bursts).
    void resetPacing() { pacingValid_ = false; }
    void paceFrame();

    FramePool* pool_;
    Instrumentation* instr_;

private:
    FrameQueue* out_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> paused_{true};
    std::atomic<bool> finished_{false};
    std::atomic<int> nativeChannels_{0};
    std::atomic<int> nativeWidth_{0};
    std::atomic<int> nativeHeight_{0};
    AtomicSharedPtr<RecordTarget> recordTarget_{nullptr}; // libc++ lacks std::atomic<shared_ptr>
    std::mutex pauseMu_;
    std::condition_variable pauseCv_;

    // playbackIntervalUs_ crosses threads (GUI write / source read); nextDeadline_ and pacingValid_
    // are source-thread only, so they need no synchronisation.
    std::atomic<double> playbackIntervalUs_{1'000'000.0 / 30.0};
    Timestamp nextDeadline_{};
    bool pacingValid_ = false;
};

} // namespace livim
