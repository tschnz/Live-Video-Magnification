#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "core/Frame.hpp"
#include "processing/IProcessor.hpp"
#include "processing/magnification/RieszPyramid.hpp"
#include "processing/magnification/SpatialFilter.hpp"
#include "processing/magnification/TemporalFilter.hpp"

// Eulerian video magnification core: per-frame ports of the reference Magnificator's
// laplaceMagnify / colorMagnify / rieszMagnify (src/main/magnification/Magnificator.cpp).
// Each magnify*() returns true and fills (out8u, outFmt) with the 8-bit result, or false to
// signal a passthrough (warmup, or a mode/input combination the reference did not process).
namespace livim::magcore {

// --- per-mode temporal state ------------------------------------------------------------------
struct MotionState {
    std::vector<cv::Mat> lowpassHi; // per pyramid level, CV_32F (levels+1 mats)
    std::vector<cv::Mat> lowpassLo;
    bool empty() const { return lowpassHi.empty(); }
    void reset() { lowpassHi.clear(); lowpassLo.clear(); }
};

struct ColorState {
    cv::Mat window; // rolling temporal window of the smallest Gaussian level, one column per frame
    void reset() { window = cv::Mat(); }
};

struct RieszState {
    std::shared_ptr<RieszPyramid> cur, old;
    std::shared_ptr<RieszTemporalFilter> lo, hi; // low-/high-cutoff Butterworth
    void reset() { cur.reset(); old.reset(); lo.reset(); hi.reset(); }
};

// Tracks the last structural parameters (mode/levels/size/channels/preprocess geometry) so the
// caller can drop temporal state on a structural change -- the equivalent of the reference GUI
// calling Magnificator::clearBuffer() on a mode/levels/ROI/grayscale change.
struct StructuralTracker {
    MagnificationMode mode = MagnificationMode::None;
    int levels = -1;
    int channels = -1;
    cv::Size size{0, 0};
    PreprocessParams preprocess{};

    // Returns true if a structural change occurred (caller must reset temporal state).
    bool update(const ProcessorConfig& cfg, int lv, int ch, cv::Size sz) {
        const MagnificationParams& p = cfg.magnification;
        const bool change = p.mode != mode || lv != levels || sz != size || ch != channels ||
                            cfg.preprocess != preprocess;
        if (change) {
            mode = p.mode;
            levels = lv;
            size = sz;
            channels = ch;
            preprocess = cfg.preprocess;
        }
        return change;
    }

    // Partial clear for the disabled/identity path; the next real frame re-syncs the rest.
    void disable() {
        mode = MagnificationMode::None;
        levels = -1;
        channels = -1;
        size = cv::Size(0, 0);
    }

    // Full clear: forces the next frame down the first-frame path from a clean slate.
    void reset() {
        disable();
        preprocess = PreprocessParams{};
    }
};

// --- Motion / Laplace (reference laplaceMagnify) ------------------------------------------------
inline bool magnifyMotion(const cv::Mat& in8u, const MagnificationParams& p, int levels,
                          int channels, MotionState& st, cv::Mat& out8u, PixelFormat& outFmt) {
    const bool color = channels >= 3;

    cv::Mat input;
    if (color) {
        in8u.convertTo(input, CV_32FC3, 1.0 / 255.0f);
        cv::cvtColor(input, input, cv::COLOR_BGR2Lab);
    } else {
        in8u.convertTo(input, CV_32FC1, 1.0 / 255.0f);
    }

    std::vector<cv::Mat> inputPyramid;
    buildLaplacePyrFromImg(input, levels, inputPyramid);

    const bool firstFrame = st.empty();
    cv::Mat output;
    if (firstFrame) {
        st.lowpassHi = inputPyramid;
        st.lowpassLo = inputPyramid;
        output = input;
    } else {
        std::vector<cv::Mat> motionPyramid(levels + 1);
        for (int curLevel = 0; curLevel < levels; ++curLevel) {
            iirFilter(inputPyramid.at(curLevel), motionPyramid.at(curLevel),
                      st.lowpassHi.at(curLevel), st.lowpassLo.at(curLevel), p.coLow, p.coHigh);
        }
        // The residual level is zeroed by the amplification below; seed it so it has the right
        // size/type (the reference kept it in a member pyramid).
        motionPyramid.at(levels) = inputPyramid.at(levels);

        const int w = input.size().width;
        const int h = input.size().height;

        const float delta =
            static_cast<float>(p.coWavelength / (8.0 * (1.0 + p.amplification)));

        // Amplification booster for better visualization (DEFAULT_LAP_MAG_EXAGGERATION)
        const float exaggeration_factor = 2.0;

        // Representative wavelength; halved for every pyramid level below.
        float lambda = static_cast<float>(std::sqrt(double(w * w + h * h)) / 3.0);

        // Zero the residual and the highest-resolution difference level, amplify the rest.
        for (int curLevel = levels; curLevel >= 0; --curLevel) {
            const float currAlpha = (lambda / (delta * 8.0) - 1.0) * exaggeration_factor;
            cv::Mat& m = motionPyramid.at(curLevel);
            m = (curLevel == levels || curLevel == 0)
                    ? m * 0
                    : m * std::min(static_cast<float>(p.amplification), currAlpha);
            lambda /= 2.0;
        }

        cv::Mat motion;
        buildImgFromLaplacePyr(motionPyramid, levels, motion);

        // Attenuate the two chrominance channels of the Lab motion image.
        if (motion.channels() > 2) {
            cv::Mat planes[3];
            cv::split(motion, planes);
            planes[1] = planes[1] * p.chromAttenuation;
            planes[2] = planes[2] * p.chromAttenuation;
            cv::merge(planes, 3, motion);
        }

        output = input + motion;
    }

    if (color) {
        cv::cvtColor(output, output, cv::COLOR_Lab2BGR);
        output.convertTo(out8u, CV_8UC3, 255.0, 1.0 / 255.0);
        outFmt = PixelFormat::BGR8;
    } else {
        output.convertTo(out8u, CV_8UC1, 255.0, 1.0 / 255.0);
        outFmt = PixelFormat::Gray8;
    }
    return true;
}

// --- Colour (reference colorMagnify: Gaussian + ideal FFT bandpass) -----------------------------
inline bool magnifyColor(const cv::Mat& in8u, const MagnificationParams& p, int levels,
                         int channels, ColorState& st, cv::Mat& out8u, PixelFormat& outFmt) {
    const bool color = channels >= 3;

    // Stays in [0,255] (NO 1/255 scaling); the output is rescaled by its min/max at the end.
    cv::Mat input;
    in8u.convertTo(input, color ? CV_32FC3 : CV_32FC1);

    std::vector<cv::Mat> inputPyramid;
    buildGaussPyrFromImg(input, levels, inputPyramid);

    // The rolling window holds one column per frame of the smallest pyramid level.
    cv::Mat downSampledFrame = inputPyramid.at(levels - 1);
    img2tempMat(downSampledFrame, st.window, getOptimalBufferSize(static_cast<int>(p.framerate)));

    // The reference's processing buffer guaranteed at least two frames in the window before the
    // first magnification; until then the raw frame was shown.
    if (st.window.cols < 2) return false;

    cv::Mat filteredMat;
    idealFilter(st.window, filteredMat, p.coLow, p.coHigh, p.framerate);

    filteredMat = filteredMat * p.amplification;

    // In the reference's steady state the reconstructed column is always index 1 of the rolling
    // window (see Magnificator's currentFrame/offset bookkeeping); with a 2-column window that is
    // also the newest column.
    cv::Mat filteredFrame;
    tempMat2img(filteredMat, std::min(1, filteredMat.cols - 1), downSampledFrame.size(),
                filteredFrame);

    cv::Mat colorImg;
    buildImgFromGaussPyr(filteredFrame, levels, colorImg, input.size());

    cv::Mat output = input + colorImg;

    // Rescale to 8-bit against the result's own min/max.
    double min, max;
    cv::minMaxLoc(output, &min, &max);
    output.convertTo(out8u, color ? CV_8UC3 : CV_8UC1, 255.0 / (max - min),
                     -min * 255.0 / (max - min));
    outFmt = color ? PixelFormat::BGR8 : PixelFormat::Gray8;
    return true;
}

// --- Riesz / Phase (reference rieszMagnify) ------------------------------------------------------
inline bool magnifyRiesz(const cv::Mat& in8u, const MagnificationParams& p, int levels,
                         int channels, RieszState& st, cv::Mat& out8u, PixelFormat& outFmt) {
    // The reference processed only multi-channel input here (grayscale mode produced no frame).
    if (channels < 3) return false;

    static const double PI_PERCENT = CV_PI / 100.0;

    // Lab, so only luminance is magnified.
    cv::Mat buffer_in;
    in8u.convertTo(buffer_in, CV_32FC3, 1.0 / 255.0);
    cv::cvtColor(buffer_in, buffer_in, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> labChannels;
    cv::split(buffer_in, labChannels);
    cv::Mat input = labChannels[0];

    // If first frame ever (or the Butterworth coefficients degenerated to NaN), init pyramids and
    // filters; the reference emitted the unmagnified frame for it.
    if (!st.cur || std::isnan(st.lo->itsA[0]) || std::isnan(st.hi->itsA[0])) {
        st.cur.reset();
        st.old.reset();
        st.lo.reset();
        st.hi.reset();
        st.cur = std::make_shared<RieszPyramid>();
        st.old = std::make_shared<RieszPyramid>();
        st.cur->init(input, levels);
        st.old->init(input, levels);
        st.lo = std::make_shared<RieszTemporalFilter>(p.coLow, p.framerate, st.cur->getSizes());
        st.hi = std::make_shared<RieszTemporalFilter>(p.coHigh, p.framerate, st.cur->getSizes());
        st.lo->computeCoefficients();
        st.hi->computeCoefficients();
        return false; // passthrough
    }

    // Recompute Butterworth coefficients when the GUI changed a cutoff.
    if (st.lo->itsFrequency != p.coLow) {
        st.lo->updateFrequency(p.coLow);
        st.lo->resetMat();
        st.hi->resetMat();
        st.old->buildPyramid(input);
    }
    if (st.hi->itsFrequency != p.coHigh) {
        st.hi->updateFrequency(p.coHigh);
        st.hi->resetMat();
        st.lo->resetMat();
        st.old->buildPyramid(input);
    }

    st.cur->buildPyramid(input);
    st.cur->computePhaseDifferenceAndAmplitude(*st.old);

    for (int lvl = 0; lvl < st.cur->numLevels - 1; ++lvl) {
        st.lo->IIRTemporalFilter(st.cur->pyrLevels[lvl].itsLowpassIIR,
                                 st.cur->pyrLevels[lvl].itsPhaseDiff, lvl);
        st.hi->IIRTemporalFilter(st.cur->pyrLevels[lvl].itsHighpassIIR,
                                 st.cur->pyrLevels[lvl].itsPhaseDiff, lvl);
    }

    // Shift current to prior for the next iteration.
    *st.old = *st.cur;

    st.cur->amplify(p.amplification, p.coWavelength * PI_PERCENT);
    cv::Mat magnified = st.cur->collapsePyramid();

    // OpenCV's floating-point Lab L range is [0,100]; one bad coefficient
    // otherwise saturates a pixel to white after Lab2BGR.
    cv::max(magnified, 0.0, magnified);
    cv::min(magnified, 100.0, magnified);

    cv::Mat output;
    magnified.convertTo(labChannels[0], CV_32FC1);
    cv::merge(labChannels, output);
    cv::cvtColor(output, output, cv::COLOR_Lab2BGR);
    output.convertTo(out8u, CV_8UC3, 255.0, 0.0);
    outFmt = PixelFormat::BGR8;
    return true;
}

} // namespace livim::magcore
