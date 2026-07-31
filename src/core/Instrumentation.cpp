#include "core/Instrumentation.hpp"

#include <algorithm>

namespace livim {

void Instrumentation::recordLatency(double ms) {
    if (ms < 0.0) ms = 0.0;
    int bucket = static_cast<int>(ms / kBucketMs);
    if (bucket >= kBuckets) bucket = kBuckets - 1;
    std::lock_guard<std::mutex> lg(histMu_);
    ++hist_[bucket];
    ++latencyCount_;
    latencySumMs_ += ms;
}

StatsSnapshot Instrumentation::snapshot() {
    StatsSnapshot s;
    s.captured = captured_.load(std::memory_order_relaxed);
    s.processed = processed_.load(std::memory_order_relaxed);
    s.displayed = displayed_.load(std::memory_order_relaxed);
    s.displaySkipped = displaySkipped_.load(std::memory_order_relaxed);
    s.sourceDrops = sourceDrops_.load(std::memory_order_relaxed);
    s.procErrors = procErrors_.load(std::memory_order_relaxed);
    s.readErrors = readErrors_.load(std::memory_order_relaxed);
    s.queueDepth = queueDepth_.load(std::memory_order_relaxed);

    const Timestamp t = now();
    if (haveLastSnapshot_) {
        const double dt = std::chrono::duration<double>(t - lastSnapshotTs_).count();
        if (dt > 0.0) {
            const std::uint64_t processedDelta = s.processed - lastProcessed_;
            const std::uint64_t dropsDelta = s.sourceDrops - lastSourceDrops_;

            const double inst = static_cast<double>(processedDelta) / dt;
            fpsEma_ = haveFpsEma_ ? (kFpsEmaAlpha * inst + (1.0 - kFpsEmaAlpha) * fpsEma_) : inst;
            haveFpsEma_ = true;

            const std::uint64_t resolved = dropsDelta + processedDelta;
            if (resolved > 0) {
                const double instDrop = static_cast<double>(dropsDelta) / static_cast<double>(resolved);
                dropEma_ = haveDropEma_ ? (kFpsEmaAlpha * instDrop + (1.0 - kFpsEmaAlpha) * dropEma_)
                                        : instDrop;
                haveDropEma_ = true;
            }
        }
    }
    s.fps = fpsEma_;
    s.dropFraction = dropEma_;

    lastSnapshotTs_ = t;
    lastProcessed_ = s.processed;
    lastSourceDrops_ = s.sourceDrops;
    haveLastSnapshot_ = true;

    {
        std::lock_guard<std::mutex> lg(histMu_);
        if (latencyCount_ > 0) {
            s.latencyMeanMs = latencySumMs_ / static_cast<double>(latencyCount_);
            const std::uint64_t target = (latencyCount_ * 95 + 99) / 100; // ceil(0.95 * n)
            std::uint64_t acc = 0;
            for (int i = 0; i < kBuckets; ++i) {
                acc += hist_[i];
                if (acc >= target) {
                    s.latencyP95Ms = (i + 0.5) * kBucketMs; // bucket center
                    break;
                }
            }
        }
    }
    return s;
}

void Instrumentation::reset() {
    captured_.store(0, std::memory_order_relaxed);
    processed_.store(0, std::memory_order_relaxed);
    displayed_.store(0, std::memory_order_relaxed);
    displaySkipped_.store(0, std::memory_order_relaxed);
    sourceDrops_.store(0, std::memory_order_relaxed);
    procErrors_.store(0, std::memory_order_relaxed);
    readErrors_.store(0, std::memory_order_relaxed);
    queueDepth_.store(0, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lg(histMu_);
    hist_.fill(0);
    latencyCount_ = 0;
    latencySumMs_ = 0.0;
    haveLastSnapshot_ = false;
    lastProcessed_ = 0;
    fpsEma_ = 0.0;
    haveFpsEma_ = false;
    lastSourceDrops_ = 0;
    dropEma_ = 0.0;
    haveDropEma_ = false;
}

} // namespace livim
