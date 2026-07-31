#pragma once

#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "processing/magnification/ComplexMat.hpp"

// Riesz pyramid for phase-based motion magnification (reference implementation,
// src/main/magnification/RieszPyramid.cpp). Single-channel CV_32FC1.
namespace livim {

void cosSin(const cv::Mat& X, CompExpMat& result);
// Element-wise inverse cosine, with X clamped into [-1, 1].
void arcCos(const cv::Mat& X, cv::Mat& result);

class RieszPyramidLevel {

public:
    RieszPyramidLevel();
    ~RieszPyramidLevel();
    RieszPyramidLevel(const RieszPyramidLevel& other);
    RieszPyramidLevel& operator=(const RieszPyramidLevel& other);

    cv::Size itsSize;
    int itsLvl;

    // Pyramid building.
    cv::Mat itsLowpass;
    ComplexMat itsRiesz;
    cv::Mat itsAmplitude;
    cv::Mat itsAmplitudeBlurred;
    // Magnification.
    CompExpMat itsPhaseDiff;
    CompExpMat itsHighpassIIR;
    CompExpMat itsLowpassIIR;

    // `octave` is a Laplace pyramid level; this applies the x and y kernels.
    void build(const cv::Mat& octave, const int lvl);

    // Movements separated by edges: cos (itsPhase.first) are vertical edges, sin (itsPhase.second)
    // are horizontal ones.
    void computePhaseDifferenceAndAmplitude(const RieszPyramidLevel& prior);

    // Multiplies this level's phase difference by alpha, up to a ceiling threshold.
    void amplify(double alpha, double threshold);

    void normalize(CompExpMat& result);
};

class RieszPyramid {
    typedef std::vector<RieszPyramidLevel>::size_type size_type;

public:
    RieszPyramid();
    ~RieszPyramid();
    RieszPyramid(const RieszPyramid& other);
    RieszPyramid& operator=(const RieszPyramid& other);

    int numLevels;
    std::vector<RieszPyramidLevel> pyrLevels;

    void init(cv::Mat& frame, int levels);
    void buildPyramid(const cv::Mat& frame);
    const cv::Mat collapsePyramid();
    void computePhaseDifferenceAndAmplitude(const RieszPyramid& prior);

    // Amplify motion by alpha up to threshold using filtered phase data.
    void amplify(double alpha, double threshold);

    cv::Size getLvlSize(int lvl);
    std::vector<std::pair<int, int>> getSizes();

private:
    // 9x9 filters for pyramid construction, applied before phase unwrapping.
    cv::Mat lowPassFilter;
    cv::Mat highPassFilter;

    // Upsample / subsample without interpolation, for pyramid collapse.
    const cv::Mat injectZerosEven(cv::Mat& img);
    const cv::Mat subsample(cv::Mat& img);
};

} // namespace livim
