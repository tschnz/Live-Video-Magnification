#pragma once

#include "core/Frame.hpp"

namespace livim {

// Laplace = Laplacian pyramid + IIR bandpass (Eulerian motion), Phase = phase-based (Riesz pyramid +
// Butterworth), Color = Gaussian pyramid + ideal FFT bandpass. None = internal bypass, not a GUI
// choice. The first three values are kept in lock-step with the GUI mode combo's item order.
enum class MagnificationMode { Laplace, Phase, Color, None };

// Magnification parameters in ALGORITHM units (UI mapping lives in MagnificationParamsUi).
// coLow/coHigh: Laplace = IIR blend coefficients in [0,1]; Color/Phase = Hz.
struct MagnificationParams {
    MagnificationMode mode = MagnificationMode::Laplace;
    double amplification    = 0.0; // alpha
    double coWavelength     = 0.0; // spatial cutoff wavelength (lambda_c analogue)
    double coLow            = 0.0;
    double coHigh           = 0.0;
    double chromAttenuation = 0.0; // chrominance (Lab a,b) attenuation, colour motion frames only
    int    levels           = 4;
    double framerate        = 30.0;// true capture rate (used by Color ideal filter & Riesz Butterworth)
};

// Geometric preprocessing applied BEFORE grayscale + magnification (see PreprocessProcessor).
struct PreprocessParams {
    int   downscale  = 1;     // divide each dimension by 1 / 2 / 4 / 8 (1 = full resolution)
    bool  roiEnabled = false; // crop to the roi* rect below before magnifying
    float roiX = 0.0f;        // ROI left   as a fraction of width  [0,1]
    float roiY = 0.0f;        // ROI top    as a fraction of height [0,1]
    float roiW = 1.0f;        // ROI width  as a fraction of width  [0,1]
    float roiH = 1.0f;        // ROI height as a fraction of height [0,1]

    // The magnification stage resets its temporal buffers whenever this changes (including a moved
    // ROI at the same size). Fields are copied verbatim, so exact comparison is correct.
    friend bool operator==(const PreprocessParams& a, const PreprocessParams& b) {
        return a.downscale == b.downscale && a.roiEnabled == b.roiEnabled && a.roiX == b.roiX &&
               a.roiY == b.roiY && a.roiW == b.roiW && a.roiH == b.roiH;
    }
    friend bool operator!=(const PreprocessParams& a, const PreprocessParams& b) { return !(a == b); }
};

// Processing parameters published from the GUI via AtomicConfig; read once per frame.
struct ProcessorConfig {
    bool grayscale = false;
    PreprocessParams preprocess;
    MagnificationParams magnification;
};

class IProcessor {
public:
    virtual ~IProcessor() = default;

    // Runs on the processing thread. Must preserve frame order and metadata (seq/pts/captureTs).
    virtual FrameRef process(const FrameRef& in, const ProcessorConfig& cfg) = 0;

    // Drop any retained temporal state so the next process() behaves as the first frame (used to
    // recover after a stage throws mid-frame). Stateless stages need do nothing.
    virtual void reset() {}
};

} // namespace livim
