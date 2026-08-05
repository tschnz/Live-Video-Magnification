#include "source/FileSource.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "core/Instrumentation.hpp"

namespace livim {

FileSource::FileSource(std::string path, FrameQueue* out, FramePool* pool, Instrumentation* instr)
    : SourceBase(out, pool, instr), path_(std::move(path)) {}

bool FileSource::openCapture(cv::VideoCapture& cap, const std::string& path, bool forceSoftware) {
    if (forceSoftware) {
        // CAP_PROP_HW_ACCELERATION is honoured only at open time and only by the FFmpeg backend.
        // On builds that ignore the parameter the open returns false, so retry with a plain open
        // rather than failing outright.
        if (cap.open(cv::String(path), static_cast<int>(cv::CAP_FFMPEG),
                     std::vector<int>{cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_NONE}))
            return true;
        cap.release();
    }
    return cap.open(path);
}

bool FileSource::open() {
    if (!openCapture(cap_, path_, /*forceSoftware=*/false)) return false;

    // Probe channels/size once (OpenCV only reveals them by decoding a frame); rewind afterwards.
    cv::Mat probe;
    if (!cap_.read(probe) || probe.empty()) {
        // The capture opened but the first frame won't decode — typically a hardware-accelerated
        // codec (e.g. AV1) with no hardware decoder, where the FFmpeg backend prints "Failed to get
        // pixel format" / "Get current frame error" and every read fails. Retry with the decoder
        // pinned to pure software before giving up.
        cap_.release();
        if (!openCapture(cap_, path_, /*forceSoftware=*/true)) return false;
        if (!cap_.read(probe) || probe.empty()) return false;
        usedSoftwareDecodeFallback_ = true;
        cap_.set(cv::CAP_PROP_POS_FRAMES, 0);
    }
    setNativeChannels(probe.channels());
    setNativeSize(probe.cols, probe.rows);

    const double fps = cap_.get(cv::CAP_PROP_FPS);
    reportedFps_ = (fps > 1.0) ? fps : 30.0; // fall back to 30 when unreported
    frameIntervalUs_ = 1'000'000.0 / reportedFps_;

    // Containers reporting a 0/garbage frame count disable the timeline (seekable() == false).
    const double count = cap_.get(cv::CAP_PROP_FRAME_COUNT);
    frameCount_ = count > 0.0 ? static_cast<std::int64_t>(count) : 0;
    outFrame_.store(-1, std::memory_order_release); // default: to the end

    pos_ = 0;
    return true;
}

std::int64_t FileSource::effectiveOut() const {
    const std::int64_t out = outFrame_.load(std::memory_order_acquire);
    if (out >= 0) return out;
    return frameCount_ > 0 ? frameCount_ : std::numeric_limits<std::int64_t>::max();
}

void FileSource::seekFrame(std::int64_t frame) {
    const std::int64_t in = inFrame_.load(std::memory_order_acquire);
    const std::int64_t hi = std::max<std::int64_t>(in, effectiveOut() - 1);
    reachedEnd_.store(false, std::memory_order_release);
    pendingSeekFrame_.store(std::clamp<std::int64_t>(frame, in, hi), std::memory_order_release);
    wakePauseWaiters(); // so a paused thread wakes and renders the scrubbed frame
}

void FileSource::setInOut(std::int64_t in, std::int64_t out) {
    const std::int64_t total = frameCount_ > 0 ? frameCount_ : std::numeric_limits<std::int64_t>::max();
    const std::int64_t o = out < 0 ? -1 : std::clamp<std::int64_t>(out, 1, total);
    const std::int64_t hi = (o < 0 ? total : o) - 1;
    inFrame_.store(std::clamp<std::int64_t>(in, 0, std::max<std::int64_t>(0, hi)),
                   std::memory_order_release);
    outFrame_.store(o, std::memory_order_release);
}

void FileSource::run() {
    while (!stopRequested()) {
        // Also wake on a pending seek so the user can scrub while paused.
        const Clock::duration paused =
            waitWhilePaused([this] { return pendingSeekFrame_.load(std::memory_order_acquire) >= 0; });
        if (stopRequested()) break;
        if (paused > Clock::duration::zero()) resetPacing();

        const std::int64_t seekTo = pendingSeekFrame_.exchange(-1, std::memory_order_acq_rel);
        const bool didSeek = seekTo >= 0;
        if (didSeek) {
            const std::int64_t maxFrame = frameCount_ > 0 ? frameCount_ - 1 : seekTo;
            pos_ = std::clamp<std::int64_t>(seekTo, 0, std::max<std::int64_t>(0, maxFrame));
            cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(pos_));
            resetPacing();
            reachedEnd_.store(false, std::memory_order_release);
        }

        const bool isPaused = this->paused();
        if (isPaused && !didSeek) continue; // paused, nothing to scrub

        // Enforce the OUT bound: loop back to IN, or park at the out-point.
        if (!isPaused && pos_ >= effectiveOut()) {
            if (loop_.load(std::memory_order_acquire)) {
                pos_ = inFrame_.load(std::memory_order_acquire);
                cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(pos_));
                resetPacing();
            } else {
                reachedEnd_.store(true, std::memory_order_release);
                currentFrame_.store(std::max<std::int64_t>(0, effectiveOut() - 1),
                                    std::memory_order_release);
                pause();
                resetPacing();
                continue;
            }
        }

        MutableFrameRef frame = acquireFrame();
        if (!frame) break; // pool stopped

        if (!cap_.read(frame->image)) {
            if (loop_.load(std::memory_order_acquire) && !isPaused) {
                pos_ = inFrame_.load(std::memory_order_acquire);
                cap_.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(pos_));
                resetPacing();
                continue;
            }
            // Natural EOF (non-looping): park at the end, stay alive so the user can still scrub.
            reachedEnd_.store(true, std::memory_order_release);
            if (frameCount_ > 0) currentFrame_.store(frameCount_ - 1, std::memory_order_release);
            pause();
            resetPacing();
            continue;
        }

        const std::int64_t emitted = pos_;
        ++pos_;

        // Drop a scrub-induced frame superseded by a newer pending seek.
        if (didSeek && pendingSeekFrame_.load(std::memory_order_acquire) >= 0) continue;

        frame->seq = seq_++;
        frame->captureTs = now();
        frame->width = frame->image.cols;
        frame->height = frame->image.rows;
        const int channels = frame->image.channels();
        frame->format = (channels == 1) ? PixelFormat::Gray8 : PixelFormat::BGR8;
        setNativeChannels(channels);
        frame->ptsUs = static_cast<std::int64_t>(static_cast<double>(emitted) * frameIntervalUs_);
        currentFrame_.store(emitted, std::memory_order_release);

        if (instr_) instr_->onCaptured();
        if (!isPaused) paceFrame(); // scrub previews emit immediately, never paced
        if (!emit(std::move(frame))) break;
    }
}

} // namespace livim
