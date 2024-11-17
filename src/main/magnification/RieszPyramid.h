#pragma once

#include "../helper/ComplexMat.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Write into result the element-wise cosines and sines of X.
static void cosSin(const cv::Mat &X, CompExpMat &result);
// Write into result the element-wise inverse cosine of X.
void arcCos(const cv::Mat &X, cv::Mat &result);
// Write into result the element-wise inverse tangent of x and y.
void arcTan2(const cv::Mat &Y, const cv::Mat &X, cv::Mat &result);
void arcTan(const cv::Mat &X, cv::Mat &result);
void calcAmplitudeAndPhase(const cv::Mat &img, CompExpMat &amplitude,
                           CompExpMat &phase, CompExpMat &orientation);

class RieszPyramidLevel {

public:
  RieszPyramidLevel();
  ~RieszPyramidLevel();
  RieszPyramidLevel(const RieszPyramidLevel &other);
  RieszPyramidLevel &operator=(const RieszPyramidLevel &other);

  cv::Size itsSize;
  int itsLvl;

  // For Pyramid Building
  cv::Mat itsLowpass;
  ComplexMat itsRiesz;
  cv::Mat itsAmplitude;
  cv::Mat itsAmplitudeBlurred;
  // For Magnification
  CompExpMat itsPhaseDiff;
  CompExpMat itsHighpassIIR;
  CompExpMat itsLowpassIIR;

  CompExpMat itsDenoisedOrients;

  // Octave is a laplace pyr level. This applies x and yKernel
  void build(const cv::Mat &octave, const int lvl);

  // This calculates movements separated by edges.
  // Cos (itsPhase.first) are vertical edges
  // Sin (itsPhase.second) are horizontal edges
  void computePhaseDifferenceAndAmplitude(const RieszPyramidLevel &prior);

  CompExpMat amplitudeWeightedBlur(const CompExpMat input);

  // Multipy the phase difference in this level by alpha but only up to
  // some ceiling threshold.
  void amplify(double alpha, double threshold);

  void denoise(CompExpMat &input, CompExpMat &result);

  void normalize(CompExpMat &result);
};

// Riesz Pyramid
//
class RieszPyramid {
  typedef std::vector<RieszPyramidLevel>::size_type size_type;

public:
  RieszPyramid();
  ~RieszPyramid();
  RieszPyramid(const RieszPyramid &other);
  RieszPyramid &operator=(const RieszPyramid &other);

  int numLevels;
  // Vector of Pyramid Levels
  std::vector<RieszPyramidLevel> pyrLevels;

  void resetMat();

  // Initialize filter and levels
  void init(cv::Mat &frame, int levels);

  // This builds a Riesz pyramid
  void buildPyramid(const cv::Mat &frame);
  // Return the frame resulting from the collapse of this pyramid.
  const cv::Mat collapsePyramid();

  // This calculates movements separated by edges.
  // Cos (itsPhase.first) are vertical edges
  // Sin (itsPhase.second) are horizontal edges
  void computePhaseDifferenceAndAmplitude(const RieszPyramid &prior);

  // Amplify motion by alpha up to threshold using filtered phase data.
  void amplify(double alpha, double threshold);

  // Returns Size(width, height) of a level
  cv::Size getLvlSize(int lvl);
  std::vector<std::pair<int, int>> getSizes();

  void denoisePhase();

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