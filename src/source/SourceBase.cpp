#include "source/SourceBase.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

#include "core/FramePool.hpp"
#include "core/IFrameSink.hpp"

namespace livim {

SourceBase::SourceBase(FrameQueue* out, FramePool* pool, Instrumentation* instr)
    : pool_(pool), instr_(instr), out_(out) {}

SourceBase::~SourceBase() { stop(); }

void SourceBase::start() {
    if (thread_.joinable()) return;
    thread_ = std::thread([this] {
        run();
        // Distinguish a natural end (EOF) from an external stop().
        finished_.store(!stop_.load(std::memory_order_acquire), std::memory_order_release);
    });
}

void SourceBase::stop() {
    stop_.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lg(pauseMu_);
        paused_.store(false, std::memory_order_release);
    }
    pauseCv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void SourceBase::play() {
    {
        std::lock_guard<std::mutex> lg(pauseMu_);
        paused_.store(false, std::memory_order_release);
    }
    pauseCv_.notify_all();
}

void SourceBase::pause() {
    std::lock_guard<std::mutex> lg(pauseMu_);
    paused_.store(true, std::memory_order_release);
}

Clock::duration SourceBase::waitWhilePaused(const std::function<bool()>& extraWake) {
    std::unique_lock<std::mutex> lk(pauseMu_);
    auto wake = [&] {
        return !paused_.load(std::memory_order_acquire) || stop_.load(std::memory_order_acquire) ||
               (extraWake && extraWake());
    };
    if (wake()) return Clock::duration::zero();
    const Timestamp start = now();
    pauseCv_.wait(lk, wake);
    return now() - start;
}

void SourceBase::wakePauseWaiters() {
    std::lock_guard<std::mutex> lg(pauseMu_);
    pauseCv_.notify_all();
}

void SourceBase::setPlaybackFps(double fps) {
    if (fps < 0.1) fps = 0.1;
    playbackIntervalUs_.store(1'000'000.0 / fps, std::memory_order_relaxed);
}

void SourceBase::setRecordTarget(std::shared_ptr<IFrameSink> sink, LatestFrameMailbox* preview) {
    auto target = std::make_shared<RecordTarget>();
    target->sink = std::move(sink);
    target->preview = preview;
    recordTarget_.store(std::move(target), std::memory_order_release);
}

void SourceBase::clearRecordTarget() {
    recordTarget_.store(nullptr, std::memory_order_release);
}

void SourceBase::paceFrame() {
    const auto interval = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double, std::micro>(
            playbackIntervalUs_.load(std::memory_order_relaxed)));
    const Timestamp t = now();
    if (!pacingValid_) {
        nextDeadline_ = t;
        pacingValid_ = true;
    }
    nextDeadline_ += interval;
    if (t >= nextDeadline_) {
        nextDeadline_ = now(); // behind: drop the deficit, resume cadence from now (never sprint)
        return;
    }
    // Sleep in slices: sleep_until is not interruptible, so cap each slice so a concurrent stop()
    // is observed within ~20 ms.
    while (!stopRequested()) {
        const Timestamp n = now();
        if (n >= nextDeadline_) break;
        std::this_thread::sleep_until(std::min(nextDeadline_, n + std::chrono::milliseconds(20)));
    }
}

MutableFrameRef SourceBase::acquireFrame() {
    if (stopRequested()) return nullptr;
    return pool_->acquire();
}

bool SourceBase::emit(FrameRef f) {
    if (stopRequested()) return false;
    return out_->push(std::move(f));
}

} // namespace livim
