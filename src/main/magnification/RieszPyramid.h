///
// From https://github.com/tbl3rd/Pyramids
///

#ifndef RIESZPYRAMID_H
#define RIESZPYRAMID_H

#include "main/helper/ComplexMat.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

class RieszPyramidLevel {

public:
    RieszPyramidLevel();
    ~RieszPyramidLevel();
    RieszPyramidLevel(const RieszPyramidLevel &other);
    RieszPyramidLevel &operator=(const RieszPyramidLevel &other);

    cv::Mat itsLp;                     // the frame scaled to this octave
    ComplexMat itsR;                   // the transform
    CompExpMat itsPhase;               // the amplified result
    CompExpMat itsRealPass;            // per-level filter state maintained
    CompExpMat itsImagPass;            // across frames

    // Octave is a laplace pyr level. This applies x and yKernel
    void build(const cv::Mat &octave);

    // Write into result the element-wise inverse cosine of X.
    static void arcCosX(const cv::Mat &X, cv::Mat &result);

    // This calculates movements separated by edges.
    // Cos (itsPhase.first) are vertical edges
    // Sin (itsPhase.second) are horizontal edges
    void unwrapOrientPhase(const RieszPyramidLevel &prior);

    // Write into result the element-wise cosines and sines of X.
    static void cosSinX(const cv::Mat &X, CompExpMat &result);

    // Used to get amplitude. Square sin&cos of phase, add lowpass, square resulting mat
    cv::Mat rms();

    // Normalize the phase change of this level into result.
    void normalize(CompExpMat &result);

    // Multipy the phase difference in this level by alpha but only up to
    // some ceiling threshold.
    void amplify(double alpha, double threshold);
};


// Riesz Pyramid
//
class RieszPyramid {
    typedef std::vector<RieszPyramidLevel>::size_type size_type;
    //RieszPyramid &operator=(const RieszPyramid &);
    //RieszPyramid(const RieszPyramid &);

public:
    RieszPyramid();
    ~RieszPyramid();
    RieszPyramid(const RieszPyramid& other);
    RieszPyramid &operator=(const RieszPyramid& other);

    int numLevels;
    // Vector of Pyramid Levels
    std::vector<RieszPyramidLevel> pyrLevels;

    // Initialize filter and levels
    void init(cv::Mat &frame, int levels);

    // This builds a Riesz pyramid
    void buildPyramid(const cv::Mat &frame);
    // Return the frame resulting from the collapse of this pyramid.
    const cv::Mat collapsePyramid();

    // This calculates movements separated by edges.
    // Cos (itsPhase.first) are vertical edges
    // Sin (itsPhase.second) are horizontal edges
    void unwrapOrientPhase(const RieszPyramid &prior);

    // Amplify motion by alpha up to threshold using filtered phase data.
    void amplify(double alpha, double threshold);

private:
    // 9x9 Lowpass and Highpass filter for pyramid construction
    // Used before phase unwrapping
    cv::Mat lowPassFilter;
    cv::Mat highPassFilter;
    // Neeed to collapse te Pyramid.
    // Upsample without interpolation
    const cv::Mat injectZerosEven(cv::Mat &img);
    // Subsample image without interpolation
    const cv::Mat subsample(cv::Mat &img);
};

#endif // RIESZPYRAMID_H
