#include "export/Exporter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "core/Clock.hpp"
#include "core/Frame.hpp"
#include "core/LatestFrameMailbox.hpp"
#include "processing/ChainBuilder.hpp"

namespace livim {
namespace {

// A frame's pixels as 3-channel BGR8 (grayscale expanded).
cv::Mat toBgr(const FrameRef& f) {
    const cv::Mat& m = f->image;
    if (m.empty()) return cv::Mat();
    if (m.channels() == 1) {
        cv::Mat bgr;
        cv::cvtColor(m, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }
    return m; // already BGR8, and callers only read it
}

void drawLabel(cv::Mat& canvas, const std::string& text, int x, int y, double scale) {
    const int font = cv::FONT_HERSHEY_SIMPLEX;
    const int thickness = std::max(1, static_cast<int>(std::lround(scale)));
    int baseline = 0;
    const cv::Size ts = cv::getTextSize(text, font, scale, thickness, &baseline);
    const int pad = std::max(2, static_cast<int>(std::lround(scale * 4)));
    cv::Rect bg(x, y, ts.width + 2 * pad, ts.height + baseline + 2 * pad);
    bg &= cv::Rect(0, 0, canvas.cols, canvas.rows);
    if (bg.width <= 0 || bg.height <= 0) return;
    cv::Mat roi = canvas(bg);
    cv::Mat dark(roi.size(), roi.type(), cv::Scalar(0, 0, 0));
    cv::addWeighted(roi, 0.35, dark, 0.65, 0.0, roi);
    cv::putText(canvas, text, cv::Point(x + pad, y + pad + ts.height), font, scale,
                cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
}

// Panes are cropped to common EVEN dimensions, which H.264/FFV1 require.
cv::Mat compose(const FrameRef& orig, const FrameRef& proc, SplitMode split, bool overlay) {
    cv::Mat p = toBgr(proc);
    if (split == SplitMode::None) {
        const int w = p.cols & ~1, h = p.rows & ~1;
        if (w <= 0 || h <= 0) return cv::Mat();
        return p(cv::Rect(0, 0, w, h)).clone();
    }
    cv::Mat o = toBgr(orig);
    if (o.empty()) o = p; // fall back if the original tap is missing
    const int w = (std::min(o.cols, p.cols)) & ~1;
    const int h = (std::min(o.rows, p.rows)) & ~1;
    if (w <= 0 || h <= 0) return cv::Mat();
    cv::Mat oc = o(cv::Rect(0, 0, w, h));
    cv::Mat pc = p(cv::Rect(0, 0, w, h));
    const double scale = std::clamp(w / 800.0, 0.4, 1.5);

    cv::Mat canvas;
    if (split == SplitMode::LeftRight) {
        canvas.create(h, 2 * w, CV_8UC3);
        oc.copyTo(canvas(cv::Rect(0, 0, w, h)));
        pc.copyTo(canvas(cv::Rect(w, 0, w, h)));
        if (overlay) {
            drawLabel(canvas, "Original", 6, 6, scale);
            drawLabel(canvas, "Processed", w + 6, 6, scale);
        }
    } else { // TopBottom
        canvas.create(2 * h, w, CV_8UC3);
        oc.copyTo(canvas(cv::Rect(0, 0, w, h)));
        pc.copyTo(canvas(cv::Rect(0, h, w, h)));
        if (overlay) {
            drawLabel(canvas, "Original", 6, 6, scale);
            drawLabel(canvas, "Processed", 6, h + 6, scale);
        }
    }
    return canvas;
}

// On success `codecOut` names the fourcc actually opened and `path` is updated to the file
// actually created, which a fallback may have changed.
bool openWriter(cv::VideoWriter& w, ExportFormat fmt, std::string& path, double fps, cv::Size size,
                std::string& codecOut) {
    auto tryOpen = [&](int fourcc, const std::string& p, const char* name) {
        if (w.open(p, fourcc, fps, size, true)) {
            codecOut = name;
            path = p;
            return true;
        }
        return false;
    };
    switch (fmt) {
    case ExportFormat::Mp4H264:
        if (tryOpen(cv::VideoWriter::fourcc('a', 'v', 'c', '1'), path, "avc1")) return true;
        if (tryOpen(cv::VideoWriter::fourcc('m', 'p', '4', 'v'), path, "mp4v")) return true;
        break;
    case ExportFormat::AviMjpg:
        if (tryOpen(cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), path, "MJPG")) return true;
        break;
    case ExportFormat::MkvFfv1:
        if (tryOpen(cv::VideoWriter::fourcc('F', 'F', 'V', '1'), path, "FFV1")) return true;
        break;
    }
    // Last resort: Motion JPEG into a sibling .avi.
    std::filesystem::path fb(path);
    fb.replace_extension(".avi");
    return tryOpen(cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fb.string(), "MJPG (fallback .avi)");
}

} // namespace

Exporter::~Exporter() {
    abort();
    join();
}

void Exporter::start(std::unique_ptr<IExportFrameSource> source, ExportRequest request,
                     LatestFrameMailbox* preview) {
    join();
    preview_ = preview;
    abort_.store(false, std::memory_order_release);
    phase_.store(ExportPhase::Processing);
    framesDone_.store(0);
    framesTotal_.store(-1);
    {
        std::lock_guard<std::mutex> lg(msgMu_);
        error_.clear();
        codecUsed_.clear();
        outputPath_.clear();
    }
    thread_ = std::thread([this, src = std::move(source), req = std::move(request)]() mutable {
        run(std::move(src), std::move(req));
    });
}

void Exporter::abort() { abort_.store(true, std::memory_order_release); }

void Exporter::join() {
    if (thread_.joinable()) thread_.join();
}

ExportProgress Exporter::progress() const {
    ExportProgress p;
    p.phase = phase_.load(std::memory_order_acquire);
    p.framesDone = framesDone_.load(std::memory_order_relaxed);
    p.framesTotal = framesTotal_.load(std::memory_order_relaxed);
    std::lock_guard<std::mutex> lg(msgMu_);
    p.error = error_;
    p.codecUsed = codecUsed_;
    p.outputPath = outputPath_;
    return p;
}

void Exporter::run(std::unique_ptr<IExportFrameSource> source, ExportRequest request) {
    auto fail = [&](const std::string& msg) {
        {
            std::lock_guard<std::mutex> lg(msgMu_);
            error_ = msg;
        }
        phase_.store(ExportPhase::Error, std::memory_order_release);
    };

    cv::VideoWriter writer;
    bool writerOpen = false;

    // RAII: on EVERY exit path (return or exception) flush+release the writer and close the source
    // exactly once; both are no-ops if already done.
    struct Finalizer {
        cv::VideoWriter& writer;
        bool& open;
        IExportFrameSource* source;
        ~Finalizer() {
            if (open) { writer.release(); open = false; }
            if (source) source->close();
        }
    } finalizer{writer, writerOpen, source.get()};

    try {
        if (!source->open()) {
            fail("Could not open the export source.");
            return;
        }

        // Capture FPS (the algorithm rate) is separate from fileFps (output cadence; 0 = follow the
        // capture rate), e.g. process a 1000 fps slow-mo at its true rate but write a 30 fps file.
        const double captureFps = request.config.magnification.framerate > 0.0
                                      ? request.config.magnification.framerate
                                      : 30.0;
        const double fileFps = request.fileFps > 0.0 ? request.fileFps : captureFps;
        framesTotal_.store(source->frameCount(), std::memory_order_relaxed);

        // Same factory the live pipeline uses, built once and fed in order so the stateful
        // temporal filters stay correct.
        std::vector<std::unique_ptr<IProcessor>> chain = buildProcessors();

        ProcessorConfig cfg = request.config;     // by value -> fixed output size for the whole file
        cfg.magnification.framerate = captureFps;

        cv::Size outSize;
        std::string outPath = request.outputPath;

        std::uint64_t seq = 0;
        const double frameIntervalUs = 1'000'000.0 / captureFps;
        cv::Mat raw;
        while (!abort_.load(std::memory_order_acquire)) {
            if (!source->next(raw)) break;
            if (raw.empty()) continue;

            auto in = std::make_shared<Frame>();
            in->seq = seq++;
            in->captureTs = now();
            in->ptsUs = static_cast<std::int64_t>(static_cast<double>(in->seq) * frameIntervalUs);
            in->width = raw.cols;
            in->height = raw.rows;
            in->format = raw.channels() == 1 ? PixelFormat::Gray8 : PixelFormat::BGR8;
            // With a preview the display reads this frame asynchronously, so it must own its pixels
            // (the next next() decodes into `raw` in place); the encode-only path can alias.
            in->image = preview_ ? raw.clone() : raw;

            FrameRef original;
            const FrameRef cur = runChainOnce(chain, in, cfg, original);

            if (preview_) {
                auto df = std::make_shared<DisplayFrame>();
                df->processed = cur;
                df->original = original;
                preview_->publish(std::move(df));
            }

            cv::Mat canvas = compose(original, cur, request.split, request.textOverlay);
            if (canvas.empty()) continue;

            if (!writerOpen) {
                outSize = canvas.size();
                std::string codec;
                if (!openWriter(writer, request.format, outPath, fileFps, outSize, codec)) {
                    fail("Could not open a video writer for the chosen format.");
                    return;
                }
                {
                    std::lock_guard<std::mutex> lg(msgMu_);
                    codecUsed_ = codec;
                    outputPath_ = outPath;
                }
                writerOpen = true;
            }
            if (canvas.size() != outSize)
                cv::resize(canvas, canvas, outSize); // defensive; sizes are fixed
            writer.write(canvas);
            framesDone_.fetch_add(1, std::memory_order_relaxed);
        }

        phase_.store(ExportPhase::Finalizing, std::memory_order_release);
        // Release BEFORE the possible partial-file remove: Windows can't delete an open file.
        const bool wroteFile = writerOpen;
        if (writerOpen) { writer.release(); writerOpen = false; }

        // Surface an empty range rather than reporting a 0-frame "success".
        if (!wroteFile && !abort_.load(std::memory_order_acquire)) {
            fail("No frames to export (empty range?).");
            return;
        }

        if (abort_.load(std::memory_order_acquire)) {
            if (wroteFile) {
                std::error_code ec;
                std::filesystem::remove(outPath, ec);
            }
            phase_.store(ExportPhase::Aborted, std::memory_order_release);
        } else {
            phase_.store(ExportPhase::Done, std::memory_order_release);
        }
    } catch (const std::exception& e) {
        // An exception escaping the worker thread would std::terminate the app.
        fail(std::string("Export failed: ") + e.what());
    } catch (...) {
        fail("Export failed: unknown error.");
    }
}

} // namespace livim
