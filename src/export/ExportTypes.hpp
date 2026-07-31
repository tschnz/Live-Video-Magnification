#pragma once

#include <string>

#include "processing/IProcessor.hpp"

namespace livim {

// How the output frame is composed: None = processed only; LeftRight / TopBottom = original +
// processed panes.
enum class SplitMode { None, LeftRight, TopBottom };

// Output container + codec. FFV1 is mathematically lossless (large files).
enum class ExportFormat { Mp4H264, AviMjpg, MkvFfv1 };

// Everything the exporter needs. The algorithm capture rate lives in `config.magnification.framerate`;
// [startFrame, endFrame) trims a file's range (a camera buffer ignores it).
struct ExportRequest {
    ProcessorConfig config;
    double          fileFps = 0.0;     // output file cadence (0 -> follow the Capture FPS)
    SplitMode       split = SplitMode::None;
    bool            textOverlay = false; // burn "Original"/"Processed" labels (split modes only)
    ExportFormat    format = ExportFormat::Mp4H264;
    std::string     outputPath;
    int             startFrame = 0;    // inclusive (file only)
    int             endFrame = -1;     // exclusive; -1 = to the end (file only)
};

enum class ExportPhase { Idle, Processing, Finalizing, Done, Aborted, Error };

// Snapshot read by the GUI on a timer.
struct ExportProgress {
    ExportPhase phase = ExportPhase::Idle;
    int         framesDone = 0;
    int         framesTotal = -1; // -1 = unknown
    std::string error;            // set when phase == Error
    std::string codecUsed;        // the fourcc actually opened (may differ after a fallback)
    std::string outputPath;       // the file actually written (may differ from the request on fallback)
};

// Extension without the leading dot.
inline const char* extensionFor(ExportFormat f) {
    switch (f) {
    case ExportFormat::Mp4H264: return "mp4";
    case ExportFormat::AviMjpg: return "avi";
    case ExportFormat::MkvFfv1: return "mkv";
    }
    return "mp4";
}

} // namespace livim
