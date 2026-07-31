#pragma once

#include <utility>
#include <vector>

#include <opencv2/core.hpp>

#include "processing/magnification/ComplexMat.hpp"

// Temporal filters for Eulerian video magnification (reference implementation,
// src/main/magnification/TemporalFilter.cpp).
namespace livim {

// First-order IIR temporal bandpass (motion): difference of two exponential lowpasses. coLow/coHigh
// are blend coefficients in [0,1] (NOT Hz), coLow < coHigh; the lowpass state buffers are carried
// frame-to-frame by the caller and updated in place. dst = lowpassHi - lowpassLo.
void iirFilter(const cv::Mat& src, cv::Mat& dst, cv::Mat& lowpassHi, cv::Mat& lowpassLo,
               double cutoffLo, double cutoffHi);

// Ideal (rectangular) temporal bandpass via FFT (colour mode). `src` rows are pixels, columns are
// successive frames; the DFT runs along each row (time). cutoffLo/cutoffHi are Hz, mapped to bins
// via the framerate. Result is min-max normalized to [0,1].
void idealFilter(const cv::Mat& src, cv::Mat& dst, double cutoffLo, double cutoffHi,
                 double framerate);

// Filter mask for the ideal bandpass: bins between the cutoffs (Hz -> bin index) pass.
void createIdealBandpassFilter(cv::Mat& filter, double cutoffLo, double cutoffHi, double framerate);

// Colour temporal window length: next power of two of max(2*fps, 16), roughly two seconds.
int getOptimalBufferSize(int fps);

// Design an order-N digital Butterworth low-pass (denominator out_a, numerator out_b).
// Wn is the cutoff normalized to Nyquist (0..1).
void butterworth(unsigned int N, double Wn, std::vector<double>& out_a, std::vector<double>& out_b);

// Per-level order-2 Butterworth IIR (Direct Form II) on the Riesz quaternionic phase. One instance
// per cutoff; the low/high difference is the temporal bandpass. State is kept per pyramid level.
class RieszTemporalFilter {

    RieszTemporalFilter& operator=(const RieszTemporalFilter&);
    RieszTemporalFilter(const RieszTemporalFilter&);

public:
    RieszTemporalFilter(double frq, double fps, std::vector<std::pair<int, int>> lvlSizes);

    double itsFrequency;
    double itsFramerate;
    std::vector<double> itsA;
    std::vector<double> itsB;

    void updateFrequency(double f);
    void computeCoefficients();

    void passEach(cv::Mat& result, const cv::Mat& phase, const cv::Mat& prior);

    void pass(CompExpMat& result, const CompExpMat& phase, const CompExpMat& prior);

    void IIRTemporalFilter(CompExpMat& result, const CompExpMat& phaseDiff, int lvl);

    void resetMat();

private:
    size_t numPyrLvls;
    std::vector<CompExpMat> itsRegister0;
    std::vector<CompExpMat> itsRegister1;
    std::vector<CompExpMat> itsPhase;
};

} // namespace livim
