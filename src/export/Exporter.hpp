#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <opencv2/core.hpp>

#include "export/ExportTypes.hpp"
#include "export/IExportFrameSource.hpp"

namespace livim {

class LatestFrameMailbox;

// Offline render+encode worker shared by both export cases (file vs. recorded camera buffer). Runs a
// FRESH magnification chain fed strictly in order (the temporal filters are stateful) on its OWN
// thread, sharing no mutable state with the live pipeline.
class Exporter {
public:
    Exporter() = default;
    ~Exporter();

    Exporter(const Exporter&) = delete;
    Exporter& operator=(const Exporter&) = delete;

    // Start an export on a background thread, taking ownership of the frame source. Call only when
    // idle or after a finished run. If `preview` is set, processed frames are published latest-wins.
    void start(std::unique_ptr<IExportFrameSource> source, ExportRequest request,
               LatestFrameMailbox* preview = nullptr);

    // Idempotent. The worker stops at the next frame boundary.
    void abort();

    // Blocks for at most one frame's work.
    void join();

    // Thread-safe.
    ExportProgress progress() const;

private:
    void run(std::unique_ptr<IExportFrameSource> source, ExportRequest request);

    LatestFrameMailbox*     preview_ = nullptr; // set before the thread starts
    std::thread             thread_;
    std::atomic<bool>       abort_{false};
    std::atomic<ExportPhase> phase_{ExportPhase::Idle};
    std::atomic<int>        framesDone_{0};
    std::atomic<int>        framesTotal_{-1};

    // Guarded by msgMu_.
    mutable std::mutex      msgMu_;
    std::string             error_;
    std::string             codecUsed_;
    std::string             outputPath_; // actual file written, may differ after a fallback
};

} // namespace livim
