#include "source/CameraSource.hpp"

#include <chrono>
#include <utility>

#include "core/IFrameSink.hpp"
#include "core/Instrumentation.hpp"
#include "core/LatestFrameMailbox.hpp"
#include "source/CameraEnumerator.hpp"

namespace livim {

CameraSource::CameraSource(int deviceIndex, FrameQueue* out, FramePool* pool, Instrumentation* instr)
    : SourceBase(out, pool, instr), deviceIndex_(deviceIndex) {}

bool CameraSource::open() {
    bool opened = false;
    for (const int api : preferredCaptureApis()) {
        if (cap_.open(deviceIndex_, api)) {
            opened = true;
            break;
        }
    }
    if (!opened) return false;

    // Bound a wedged grab so teardown (which joins this thread) can't hang: a stuck cap_.read()
    // returns false after the timeout. Not every backend honors this; harmless no-op where unsupported.
    cap_.set(cv::CAP_PROP_READ_TIMEOUT_MSEC, 5000);

    // Webcam backends often report 0/garbage FPS; fall back to 30 to keep pacing sane.
    const double fps = cap_.get(cv::CAP_PROP_FPS);
    reportedFps_ = (fps > 1.0) ? fps : 30.0;

    // Probe channels/size once (OpenCV only reveals them by decoding a frame).
    cv::Mat probe;
    if (cap_.read(probe)) {
        setNativeChannels(probe.channels());
        setNativeSize(probe.cols, probe.rows);
    }
    return true;
}

void CameraSource::run() {
    // Never paced: reading slower than the hardware rate would grow latency and make the driver
    // silently drop frames.
    while (!stopRequested()) {
        waitWhilePaused();
        if (stopRequested()) break;

        MutableFrameRef frame = acquireFrame();
        if (!frame) break; // pool stopped

        if (!cap_.read(frame->image)) {
            if (stopRequested()) break;
            if (instr_) instr_->onSourceReadError();
            continue; // assume a transient read failure and try again
        }

        frame->seq = seq_++;
        frame->captureTs = now();
        frame->width = frame->image.cols;
        frame->height = frame->image.rows;
        const int channels = frame->image.channels();
        frame->format = (channels == 1) ? PixelFormat::Gray8 : PixelFormat::BGR8;
        setNativeChannels(channels);
        // No source PTS for a live feed; use the capture instant.
        frame->ptsUs = std::chrono::duration_cast<std::chrono::microseconds>(
                           frame->captureTs.time_since_epoch())
                           .count();

        if (instr_) instr_->onCaptured();

        if (const auto target = recordTarget()) {
            if (target->sink) target->sink->append(frame->image);
            if (target->preview) {
                auto df = std::make_shared<DisplayFrame>();
                df->processed = frame; // same raw frame in both panes while recording
                df->original = frame;
                target->preview->publish(std::move(df));
            }
            continue; // do NOT emit() -> lossless
        }

        if (!emit(std::move(frame))) break;
    }
}

} // namespace livim
