#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include "core/Clock.hpp"

namespace livim {

// Hardcoded instead of std::hardware_destructive_interference_size, whose value GCC warns is
// ABI-unstable (-Winterference-size); 64 is correct for mainstream x86-64/arm64.
inline constexpr std::size_t kCacheLine = 64;

// Pipeline health, polled by the GUI on a timer.
struct StatsSnapshot {
    std::uint64_t captured = 0;
    std::uint64_t processed = 0;       // processed == captured  =>  zero pipeline drops
    std::uint64_t displayed = 0;
    std::uint64_t displaySkipped = 0;  // cosmetic: processed frames overwritten before display
    std::uint64_t sourceDrops = 0;     // must stay 0 on the lossless path
    std::uint64_t procErrors = 0;      // frames a processing stage threw on (degraded, not crashed)
    std::uint64_t readErrors = 0;
    std::size_t   queueDepth = 0;
    double        fps = 0.0;           // processed frames/sec since the previous snapshot
    double        latencyMeanMs = 0.0; // capture -> processed
    double        latencyP95Ms = 0.0;
    double        dropFraction = 0.0;  // EMA of dropped/(dropped+processed)
};

// Counters are cache-line padded to avoid false sharing between the threads that bump them.
class Instrumentation {
public:
    void onCaptured() { captured_.fetch_add(1, std::memory_order_relaxed); }
    void onProcessed() { processed_.fetch_add(1, std::memory_order_relaxed); }
    void onDisplayed() { displayed_.fetch_add(1, std::memory_order_relaxed); }
    void addDisplaySkipped(std::uint64_t n) {
        displaySkipped_.fetch_add(n, std::memory_order_relaxed);
    }
    void setQueueDepth(std::size_t d) { queueDepth_.store(d, std::memory_order_relaxed); }
    void setSourceDrops(std::uint64_t d) { sourceDrops_.store(d, std::memory_order_relaxed); }
    void onProcessingError() { procErrors_.fetch_add(1, std::memory_order_relaxed); }
    void onSourceReadError() { readErrors_.fetch_add(1, std::memory_order_relaxed); }

    void recordLatency(double ms);
    StatsSnapshot snapshot(); // also computes fps over the interval since the last call
    void reset();

private:
    alignas(kCacheLine) std::atomic<std::uint64_t> captured_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> processed_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> displayed_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> displaySkipped_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> sourceDrops_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> procErrors_{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> readErrors_{0};
    alignas(kCacheLine) std::atomic<std::size_t> queueDepth_{0};

    static constexpr int kBuckets = 64;
    static constexpr double kBucketMs = 5.0; // covers 0..320 ms; last bucket is a catch-all

    std::mutex histMu_;
    std::array<std::uint64_t, kBuckets> hist_{};
    std::uint64_t latencyCount_ = 0;
    double latencySumMs_ = 0.0;

    Timestamp lastSnapshotTs_{};
    std::uint64_t lastProcessed_ = 0;
    bool haveLastSnapshot_ = false;

    // EMA state below is touched only on the GUI thread, inside snapshot().
    static constexpr double kFpsEmaAlpha = 0.3;
    double fpsEma_ = 0.0;
    bool haveFpsEma_ = false;

    std::uint64_t lastSourceDrops_ = 0;
    double dropEma_ = 0.0;
    bool haveDropEma_ = false;
};

} // namespace livim
