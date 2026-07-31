#pragma once

#include <algorithm> // std::clamp; libc++ does not leak this transitively
#include <cmath>

#include "processing/IProcessor.hpp" // MagnificationMode, MagnificationParams

namespace livim {

// The single UI<->algorithm mapping for the magnification parameters, shared by the processing panel
// and the export dialog -- it must not drift, or an exported result diverges from the preview.
// low/high are Hz in every mode; Laplace's IIR consumes a blend coefficient, so toParams/toUi
// convert Hz<->blend via `captureFps`.
struct MagUiValues {
    MagnificationMode mode = MagnificationMode::Laplace;
    int    amplification = 20;
    double wavelength = 50.0;
    double low = 1.0;  // Hz
    double high = 2.5; // Hz
    int    chroma = 0;
    int    levels = 4;
    double captureFps = 30.0; // algorithm framerate; drives every mode's Hz<->algorithm mapping
};

// Laplace temporal band: Hz <-> IIR blend coefficient via the EMA cutoff relation
// a = 1 - exp(-2*pi*fc/fps); invertible on [0 Hz, Nyquist] (the panel clamps to that range).
inline constexpr double kTwoPi = 6.283185307179586;

inline double motionHzToBlend(double hz, double fps) {
    if (fps <= 0.0) fps = 30.0;
    if (hz <= 0.0) return 0.0;
    const double a = 1.0 - std::exp(-kTwoPi * hz / fps);
    return std::clamp(a, 0.0, 0.999999); // strictly < 1 so (1 - a) stays a valid lowpass gain
}

inline double motionBlendToHz(double blend, double fps) {
    if (fps <= 0.0) fps = 30.0;
    blend = std::clamp(blend, 0.0, 0.999999);
    if (blend <= 0.0) return 0.0;
    return -(fps / kTwoPi) * std::log(1.0 - blend);
}

// Per-mode defaults (the reference's DEFAULT_MM_*). Callers seed `captureFps` from the source.
inline MagUiValues defaultsFor(MagnificationMode mode) {
    MagUiValues v;
    v.mode = mode;
    switch (mode) {
    case MagnificationMode::Color:
        v.amplification = 100;
        v.low = 0.84;
        v.high = 1.43;
        v.levels = 3;
        break;
    case MagnificationMode::Phase:
        v.amplification = 50;
        v.wavelength = 50.0;
        v.low = 1.0;
        v.high = 5.0;
        v.levels = 5;
        break;
    case MagnificationMode::Laplace:
    case MagnificationMode::None:
        v.amplification = 20;
        v.wavelength = 50.0;
        v.low = 1.0;
        v.high = 5.0;
        v.chroma = 0;
        v.levels = 4;
        break;
    }
    return v;
}

inline MagnificationParams toParams(const MagUiValues& v) {
    MagnificationParams p;
    p.mode = v.mode;
    p.amplification = v.amplification;
    p.levels = v.levels;
    p.framerate = v.captureFps;
    switch (v.mode) {
    case MagnificationMode::Color:
        p.coWavelength = 0.0;
        p.coLow = v.low;   // Hz
        p.coHigh = v.high; // Hz
        p.chromAttenuation = 0.0;
        break;
    case MagnificationMode::Laplace:
        p.coWavelength = v.wavelength * 10.0;               // UI % -> algorithm units
        p.coLow = motionHzToBlend(v.low, v.captureFps);     // UI Hz -> blend coefficient [0,1]
        p.coHigh = motionHzToBlend(v.high, v.captureFps);
        p.chromAttenuation = v.chroma / 100.0;
        break;
    case MagnificationMode::Phase:
        p.coWavelength = 100.0 - v.wavelength; // inverted so the slider matches Laplace's sense
        p.coLow = v.low;                       // Hz
        p.coHigh = v.high;                     // Hz
        p.chromAttenuation = 0.0;
        break;
    case MagnificationMode::None:
        break;
    }
    return p;
}

inline MagUiValues toUi(const MagnificationParams& p) {
    MagUiValues v;
    // None is an internal bypass, never a UI choice -> show as Laplace.
    v.mode = p.mode == MagnificationMode::None ? MagnificationMode::Laplace : p.mode;
    v.amplification = static_cast<int>(p.amplification);
    v.levels = p.levels;
    v.captureFps = p.framerate;
    switch (v.mode) {
    case MagnificationMode::Color:
        v.low = p.coLow;
        v.high = p.coHigh;
        break;
    case MagnificationMode::Laplace:
        v.wavelength = p.coWavelength / 10.0;
        v.low = motionBlendToHz(p.coLow, p.framerate);  // blend coefficient -> Hz
        v.high = motionBlendToHz(p.coHigh, p.framerate);
        v.chroma = static_cast<int>(p.chromAttenuation * 100.0);
        break;
    case MagnificationMode::Phase:
        v.wavelength = 100.0 - p.coWavelength;
        v.low = p.coLow;
        v.high = p.coHigh;
        break;
    case MagnificationMode::None:
        break;
    }
    return v;
}

} // namespace livim
