#pragma once

#include <cstdint>

#include <opencv2/videoio.hpp>

#include "source/SourceBase.hpp"

namespace livim {

// Captures from a webcam, free-running at the camera's delivery rate, pushing every frame losslessly
// (BLOCK). Do NOT set CAP_PROP_BUFFERSIZE=1: dropping frames corrupts the temporal magnification.
class CameraSource : public SourceBase {
public:
    CameraSource(int deviceIndex, FrameQueue* out, FramePool* pool, Instrumentation* instr);

    SourceKind kind() const override { return SourceKind::Camera; }
    bool open() override;
    bool isOpen() const override { return cap_.isOpened(); }
    double reportedFps() const override { return reportedFps_; }

protected:
    void run() override;

private:
    int deviceIndex_;
    cv::VideoCapture cap_;
    std::uint64_t seq_ = 0;
    double reportedFps_ = 0.0;
};

} // namespace livim
