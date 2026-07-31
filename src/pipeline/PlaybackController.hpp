#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "core/AtomicConfig.hpp"
#include "core/FramePool.hpp"
#include "core/Instrumentation.hpp"
#include "core/LatestFrameMailbox.hpp"
#include "core/PipelineTypes.hpp"
#include "processing/IProcessor.hpp"

namespace livim {

class ISource;
class ProcessingChain;
class IVideoRenderer;
class RecordingBuffer;

// Owns and wires the whole pipeline: frame pool -> source thread -> queue -> processing
// thread -> latest-wins mailbox -> renderer. Qt-free: the GUI calls these methods and polls
// stats(); no per-frame data ever crosses a Qt signal/slot.
class PlaybackController {
public:
    PlaybackController();
    ~PlaybackController();

    // The controller keeps the handle so it can unbind on destruction, whichever of the two
    // outlives the other.
    void bindRenderer(IVideoRenderer* renderer);

    bool openFile(const std::string& path);
    bool openCamera(int deviceIndex);

    void play();
    void pause();
    void stop();

    bool isPlaying();

    // Remembered across Stop -> Play; applied immediately. No-op for a camera.
    void setLoop(bool enabled);

    // Active source's raw native rate; 0 if no source. Seeds UI defaults only.
    double reportedFps();

    // Remembered across Stop -> Play; applied immediately. A freshly opened source defaults to
    // its own reported FPS.
    void setPlaybackFps(double fps);

    // Timeline / seeking (frame domain). Safe no-ops with no source.
    bool seekable();
    std::int64_t frameCount();
    std::int64_t currentFrame();
    void seekFrame(std::int64_t frame);
    void setInOut(std::int64_t in, std::int64_t out); // out exclusive; -1 = to the end

    // The target FPS, or the reported rate when unset.
    double playbackFps();

    // True when a finite source has played to its end and is parked there.
    bool atEnd();

    void publishConfig(ProcessorConfig cfg) { config_.publish(std::move(cfg)); }
    StatsSnapshot stats();

    // --- Processing controls ---

    // Live via AtomicConfig; no chain rebuild. Remembered across source opens and rebuilds.
    void setGrayscale(bool enabled);
    bool grayscaleEnabled();

    // Channels the active source produces (1 = already grayscale, 3 = BGR); 0 if no frame yet.
    int sourceChannels();

    // Eulerian magnification parameters, in algorithm units. Live via AtomicConfig; no chain
    // rebuild (the MagnificationProcessor self-resets on structural changes). Remembered.
    void setMagnification(const MagnificationParams& params);
    MagnificationParams magnification();

    // Enable/disable the magnification stage without losing mode/params. Live; no rebuild.
    void setMagnificationActive(bool active);

    // Maximum pyramid levels the current source's frame size supports (0 if no source/frame yet).
    int maxPyramidLevels();

    // Geometric preprocessing. Live via AtomicConfig; no chain rebuild. Remembered.
    // Divisor 1 = full resolution, 2/4/8 = process at 1/2..1/8 of each dimension; the ROI rect is
    // normalized [0,1] against the source frame.
    void setDownscale(int divisor);
    void setRoi(float x, float y, float w, float h);
    void clearRoi();

    PreprocessParams preprocess();

    // --- Camera lossless recording (for export) ---

    // Routes the camera into `buffer` with a raw preview to the display, bypassing the
    // magnification chain. No-op unless a camera is open.
    void beginCameraRecording(std::shared_ptr<RecordingBuffer> buffer);

    // Stops accepting frames and quiesces the source; the caller then takes the frames out of
    // its own reference to the buffer.
    void endCameraRecording();

    LatestFrameMailbox* mailbox() { return &mailbox_; }
    Instrumentation* instrumentation() { return &instr_; }

private:
    enum class State { Idle, Playing, Paused, Stopped };

    using SourceFactory = std::function<std::unique_ptr<ISource>()>;

    bool buildAndStart();    // create source+chain from factory_, start them (paused)
    void teardownThreads();  // stop+join source+chain, clear mailbox (keeps factory_)

    // Compose the live ProcessorConfig from the remembered preferences. Caller must hold mu_.
    ProcessorConfig composeConfig() const;

    // Lock mu_, apply the mutation to the remembered state, republish the composed config.
    template <class F>
    void mutateConfig(F&& mutate) {
        std::lock_guard<std::mutex> lg(mu_);
        mutate();
        config_.publish(composeConfig());
    }

    static constexpr std::size_t kPoolCapacity = 12; // in-flight frame bound
    static constexpr std::size_t kQueueCapacity = 2; // shallow source->processing queue

    FramePool pool_;
    FrameQueue queue_;
    LatestFrameMailbox mailbox_;
    Instrumentation instr_;
    AtomicConfig<ProcessorConfig> config_;

    SourceFactory factory_; // remembers how to (re)create the current source for Stop -> Play
    std::unique_ptr<ISource> source_;
    std::unique_ptr<ProcessingChain> chain_;
    IVideoRenderer* renderer_ = nullptr;

    State state_ = State::Idle;

    // Remembered preferences: re-applied on every source build and mirrored into config_.
    bool loop_ = false;
    bool cameraSource_ = false; // true => camera (queue uses Drop); false => file (Block/lossless)
    bool grayscale_ = false;
    PreprocessParams preprocess_;
    MagnificationParams magParams_;
    bool magnifyActive_ = true; // false (original-only view) -> bypass magnification
    double playbackFps_ = 0.0;  // 0 = follow the source's reported FPS
    double reportedFps_ = 0.0;

    std::shared_ptr<RecordingBuffer> recordBuf_; // non-null only while camera recording
    std::mutex mu_;
};

} // namespace livim
