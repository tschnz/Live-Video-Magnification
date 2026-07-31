#pragma once

// Qt-free mapping from a pipeline stats snapshot to status-bar health (colour).
// A live camera is judged by the share of frames it sheds (dropFraction), not by rate vs CAP_PROP_FPS.
namespace livim::statushealth {

enum class Health { Idle, Ok, Warn, Bad };

inline constexpr double kSpeedOk = 0.95;    // file: achieved/target at/above this -> ok
inline constexpr double kSpeedWarn = 0.80;  // above this but below kSpeedOk -> warn, else bad
inline constexpr double kDropWarn = 0.02;   // camera shedding >2% of frames -> warn
inline constexpr double kDropBad = 0.15;    // camera shedding >15% -> bad

struct Inputs {
    bool   live = false;         // a source is open AND frames are flowing
    bool   camera = false;       // true = live camera; false = file
    double fps = 0.0;            // processed fps (EMA)
    double targetFps = 0.0;      // file playback target (0 = unknown); unused for a camera
    double dropFraction = 0.0;   // EMA share of frames shed before processing (0 for a file)
};

// Severity of a camera's shed-share.
inline Health dropHealth(double frac) {
    if (frac < kDropWarn) return Health::Ok;
    if (frac < kDropBad) return Health::Warn;
    return Health::Bad;
}

// A camera keeps up iff it isn't shedding frames; a file is judged against its paced cadence.
inline Health speed(const Inputs& in) {
    if (!in.live) return Health::Idle;
    if (in.camera) return dropHealth(in.dropFraction);
    if (in.targetFps <= 0.0) return Health::Ok;
    const double r = in.fps / in.targetFps;
    return r >= kSpeedOk ? Health::Ok : (r >= kSpeedWarn ? Health::Warn : Health::Bad);
}

} // namespace livim::statushealth
