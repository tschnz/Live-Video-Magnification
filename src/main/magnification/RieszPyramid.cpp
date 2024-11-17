#include "RieszPyramid.h"

// Write into result the element-wise inverse cosine of X.
void arcCos(const cv::Mat &X, cv::Mat &result) {
  assert(X.isContinuous() && result.isContinuous());
  const float *const pX = X.ptr<float>(0);
  float *const pResult = result.ptr<float>(0);
  const int count = X.rows * X.cols;

  for (int i = 0; i < count; ++i) {
    if (pX[i] < -1.0) {
      pResult[i] = -1.0;
    } else if (pX[i] > 1.0) {
      pResult[i] = 1.0;
    } else {
      pResult[i] = acosf(pX[i]);
    }
  }
}

// Write into result the element-wise cosines and sines of X.
void cosSin(const cv::Mat &X, CompExpMat &result) {
  assert(X.isContinuous());
  cos(result) = cv::Mat::zeros(X.size(), CV_32FC1);
  sin(result) = cv::Mat::zeros(X.size(), CV_32FC1);
  assert(cos(result).isContinuous() && sin(result).isContinuous());
  const float *const pX = X.ptr<float>(0);
  float *const pCosX = cos(result).ptr<float>(0);
  float *const pSinX = sin(result).ptr<float>(0);
  const int count = X.rows * X.cols;
  for (int i = 0; i < count; ++i) {
    pCosX[i] = cosf(pX[i]);
    pSinX[i] = sinf(pX[i]);
  }
}

void arcTan2(const cv::Mat &Y, const cv::Mat &X, cv::Mat &result) {
  assert(X.isContinuous() && Y.isContinuous());
  result = cv::Mat::zeros(X.size(), CV_32FC1);
  assert(result.isContinuous());

  const float *const pY = Y.ptr<float>(0);
  const float *const pX = X.ptr<float>(0);
  float *const pResult = result.ptr<float>(0);
  const int count = X.rows * X.cols;

  for (int i = 0; i < count; ++i) {
    pResult[i] = atan2f(pY[i], pX[i]);
  }
}

void arcTan(const cv::Mat &X, cv::Mat &result) {
  assert(X.isContinuous());
  result = cv::Mat::zeros(X.size(), CV_32FC1);
  assert(result.isContinuous());

  const float *const pX = X.ptr<float>(0);
  float *const pResult = result.ptr<float>(0);
  const int count = X.rows * X.cols;

  for (int i = 0; i < count; ++i) {
    pResult[i] = atanf(pX[i]);
  }
}

void monogenicSignal(const cv::Mat &img, CompExpMat &amplitude,
                     CompExpMat &phase, CompExpMat &orientation) {
  // Filter Taps
  float G_f1_data[9] = {0.0094f,  0.1148f, 0.3964f, -0.0601f, -0.9213f,
                        -0.0601f, 0.3964f, 0.1148f, 0.0094f};
  cv::Mat G_f1 = cv::Mat(1, 9, CV_32FC1, G_f1_data);

  float G_f2_data[9] = {0.0008f, 0.0176f, 0.1660f, 0.6383f, 1.0000f,
                        0.6383f, 0.1660f, 0.0480f, 0.0008f};
  cv::Mat G_f2 = cv::Mat(1, 9, CV_32FC1, G_f2_data);

  float H_f1_data[9] = {-0.0098f, -0.0618f, 0.0998f, 0.7551f, 0.0000f,
                        -0.7551f, -0.0998f, 0.0618f, 0.0098f};
  cv::Mat H_f1 = cv::Mat(1, 9, CV_32FC1, H_f1_data);

  float H_f2_data[9] = {0.0008f, 0.0176f, 0.1660f, 0.6383f, 1.0000f,
                        0.6383f, 0.1660f, 0.0176f, 0.0008f};
  cv::Mat H_f2 = cv::Mat(1, 9, CV_32FC1, H_f2_data);

  // Create Monogenic Signal from real/imag pairs for vertical and horizontal
  CompExpMat horiLocals, vertLocals;
  cv::sepFilter2D(img, real(horiLocals), CV_32FC1, G_f1, G_f2,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::sepFilter2D(img, imag(horiLocals), CV_32FC1, H_f1, H_f2,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::sepFilter2D(img, real(vertLocals), CV_32FC1, G_f2, G_f1,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::sepFilter2D(img, imag(vertLocals), CV_32FC1, H_f2, H_f1,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);

  // HORIZONTAL
  cv::Mat hori_squared = square(horiLocals);
  cv::Mat hori_quadraturePair;
  cv::sqrt(hori_squared, hori_quadraturePair);
  // Amplitude
  cv::sqrt(img.mul(img) + hori_squared, hori(amplitude));
  // Phase
  arcTan2(hori_quadraturePair, img, hori(phase));
  // Phase Orientation
  arcTan2(imag(horiLocals), real(horiLocals), hori(orientation));

  // HORIZONTAL
  cv::Mat vert_squared = square(vertLocals);
  cv::Mat vert_quadraturePair;
  cv::sqrt(vert_squared, vert_quadraturePair);
  // Amplitude
  cv::sqrt(img.mul(img) + vert_squared, vert(amplitude));
  // Phase
  arcTan2(vert_quadraturePair, img, vert(phase));
  // Phase Orientation
  arcTan2(imag(vertLocals), real(vertLocals), vert(orientation));
}

/////////////////////
// Riesz Pyr Level //
/////////////////////
RieszPyramidLevel::RieszPyramidLevel() {}
RieszPyramidLevel::~RieszPyramidLevel() {}
RieszPyramidLevel::RieszPyramidLevel(const RieszPyramidLevel &other) {
  other.itsLowpass.copyTo(itsLowpass);
  real(other.itsRiesz).copyTo(real(itsRiesz));
  imag(other.itsRiesz).copyTo(imag(itsRiesz));
  cos(other.itsPhaseDiff).copyTo(cos(itsPhaseDiff));
  sin(other.itsPhaseDiff).copyTo(sin(itsPhaseDiff));
  other.itsAmplitude.copyTo(itsAmplitude);
  other.itsAmplitudeBlurred.copyTo(itsAmplitudeBlurred);
}

RieszPyramidLevel &
RieszPyramidLevel::operator=(const RieszPyramidLevel &other) {
  if (this != &other) {
    other.itsLowpass.copyTo(itsLowpass);
    real(other.itsRiesz).copyTo(real(itsRiesz));
    imag(other.itsRiesz).copyTo(imag(itsRiesz));
    cos(other.itsPhaseDiff).copyTo(cos(itsPhaseDiff));
    sin(other.itsPhaseDiff).copyTo(sin(itsPhaseDiff));
    other.itsAmplitude.copyTo(itsAmplitude);
    other.itsAmplitudeBlurred.copyTo(itsAmplitudeBlurred);
  }

  return *this;
}

// Octave is a laplace pyr level. This applies x and yKernel
void RieszPyramidLevel::build(const cv::Mat &octave, const int lvl) {
  itsSize = octave.size();
  itsLvl = lvl;
  // This is the Riesz Band Filter, sometimes defined as [-0.5, 0 , 0.5],
  // [-0.2,-0.48, 0, 0.48,0.2], [[-0.12,0,0.12],[-0.34, 0, 0.34],[-0.12,0,0.12]]
  static const cv::Mat realK =
      (cv::Mat_<float>(1, 5) << -0.2, -0.48, 0, 0.48, 0.2);
  static const cv::Mat imagK = realK.t();
  itsLowpass = octave;
  cv::filter2D(itsLowpass, real(itsRiesz), itsLowpass.depth(), realK,
               cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::filter2D(itsLowpass, imag(itsRiesz), itsLowpass.depth(), imagK,
               cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
}

// Rewrote unwrapOrientPhase
void RieszPyramidLevel::computePhaseDifferenceAndAmplitude(
    const RieszPyramidLevel &prior) {
  // See https://people.csail.mit.edu/nwadhwa/riesz-pyramid/pseudocode.pdf
  // Create conjugated quat
  cv::Mat qConjProdReal = itsLowpass.mul(prior.itsLowpass) +
                          cos(itsRiesz).mul(cos(prior.itsRiesz)) +
                          sin(itsRiesz).mul(sin(prior.itsRiesz));

  CompExpMat qConjProd =
      (prior.itsRiesz * (itsLowpass * (-1.f))) + (itsRiesz * prior.itsLowpass);

  // Quaternion Logarithm
  cv::Mat qConjProdXYsquared = square(qConjProd);
  cv::Mat qConjProdAmplitude;
  cv::sqrt(qConjProdReal.mul(qConjProdReal) + qConjProdXYsquared,
           qConjProdAmplitude);

  cv::Mat phaseDifference_tmp;
  cv::divide(qConjProdReal, qConjProdAmplitude, phaseDifference_tmp);

  cv::Mat phaseDifference =
      cv::Mat(phaseDifference_tmp.size(), phaseDifference_tmp.type());
  arcCos(phaseDifference_tmp, phaseDifference);

  cv::Mat qConjProdXYsquaredSqrt;
  cv::sqrt(qConjProdXYsquared, qConjProdXYsquaredSqrt);

  CompExpMat orientation = qConjProd / qConjProdXYsquaredSqrt;

  // Quaternionic Phase
  itsPhaseDiff = orientation * phaseDifference;
  patchNaNs(cos(itsPhaseDiff), 0.0);
  patchNaNs(sin(itsPhaseDiff), 0.0);

  sqrt(qConjProdAmplitude, itsAmplitude);

  cv::GaussianBlur(itsAmplitude, itsAmplitudeBlurred, cv::Size(13, 13), 3.0);
}

CompExpMat RieszPyramidLevel::amplitudeWeightedBlur(const CompExpMat input) {
  CompExpMat result = input * itsAmplitude;

  cv::GaussianBlur(cos(result), cos(result), cv::Size(13, 13), 3.0, 0.0,
                   cv::BORDER_REFLECT_101);
  cv::GaussianBlur(sin(result), sin(result), cv::Size(13, 13), 3.0, 0.0,
                   cv::BORDER_REFLECT_101);

  cv::divide(cos(result), itsAmplitudeBlurred, cos(result));
  cv::divide(sin(result), itsAmplitudeBlurred, sin(result));

  patchNaNs(cos(result), 0.0);
  patchNaNs(sin(result), 0.0);

  return result;
}

// Normalize the phase change of this level into result.
void RieszPyramidLevel::normalize(CompExpMat &result) {
  static const double sigma = 3.0;
  static const int aperture = static_cast<int>(1.0 + 4.0 * sigma);
  static const cv::Mat kernel =
      cv::getGaussianKernel(aperture, sigma, CV_32FC1);
  const CompExpMat change = itsHighpassIIR - itsLowpassIIR;
  cos(result) = cos(change).mul(itsAmplitude);
  sin(result) = sin(change).mul(itsAmplitude);
  cv::sepFilter2D(cos(result), cos(result), -1, kernel, kernel,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::sepFilter2D(sin(result), sin(result), -1, kernel, kernel,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::divide(cos(result), itsAmplitudeBlurred, cos(result));
  cv::divide(sin(result), itsAmplitudeBlurred, sin(result));
}

// Multipy the phase difference in this level by alpha but only up to
// some ceiling threshold.
void RieszPyramidLevel::amplify(double alpha, double threshold) {
  CompExpMat temp;
  normalize(temp);

  cv::Mat MagV = square(temp);
  cv::sqrt(MagV, MagV);
  cv::Mat MagV2 = MagV * alpha;
  cv::threshold(MagV2, MagV2, threshold, 0, cv::THRESH_TRUNC);
  CompExpMat phaseDiff;
  cosSin(MagV2, phaseDiff);
  cv::Mat pair = real(itsRiesz).mul(cos(temp)) + imag(itsRiesz).mul(sin(temp));
  cv::divide(pair, MagV, pair);
  patchNaNs(pair, 0.0);

  itsLowpass = itsLowpass.mul(cos(phaseDiff)) - pair.mul(sin(phaseDiff));
}

/// This is the way to go defined by the original MIT paper.
// void RieszPyramidLevel::amplify(double alpha, double threshold)
//{
//     CompExpMat bandwithIIR = itsHighpassIIR - itsLowpassIIR;

//    CompExpMat smoothBandwithIIR = amplitudeWeightedBlur(bandwithIIR);

//    CompExpMat amplifiedPhaseChange = smoothBandwithIIR * alpha;

//    cv::Mat phaseMagnitude = square(amplifiedPhaseChange);
//    cv::sqrt(phaseMagnitude, phaseMagnitude);

//    dbgMat(phaseMagnitude, "phaseMagnitude", this->itsLvl);
//    //std::cout << "Thresh: " << threshold << std::endl;
//    cv::threshold(phaseMagnitude, phaseMagnitude, threshold, 0,
//    cv::THRESH_TRUNC); dbgMat(phaseMagnitude, "phaseMagnitudeThresh",
//    this->itsLvl);

//    CompExpMat expPhaseRealImag;
//    cosSinX(phaseMagnitude, expPhaseRealImag);

//    CompExpMat expPhase = (amplifiedPhaseChange / phaseMagnitude) *
//    imag(expPhaseRealImag);
//    //patchNaNs(sin(expPhase), 0.0);
//    //patchNaNs(cos(expPhase), 0.0);

//    // Threshold to prevent undershooting of Lowpass
//    double cMin,cMax;
//    cv::minMaxLoc(cos(itsRiesz), &cMin, &cMax);
//    cv::Mat cTmp = cos(expPhase).mul(cos(itsRiesz));
//    cv::threshold(cTmp, cTmp, cMin, cMin, cv::THRESH_TOZERO);
//    double sMin,sMax;
//    cv::minMaxLoc(sin(itsRiesz), &sMin, &sMax);
//    cv::Mat sTmp = sin(expPhase).mul(sin(itsRiesz));
//    cv::threshold(sTmp, sTmp, sMin, sMin, cv::THRESH_TOZERO);

//    itsLowpass = real(expPhaseRealImag).mul(itsLowpass)
//                    - cTmp
//                    - sTmp;
//}

void RieszPyramidLevel::denoise(CompExpMat &input, CompExpMat &result) {
  // Create gaussian
  static const double sigma = 3.0;
  static const int aperture = static_cast<int>(1.0 + 4.0 * sigma);
  static const cv::Mat kernel =
      cv::getGaussianKernel(aperture, sigma, CV_32FC1);
  // Get amplitude
  // Weigh phase diff with amplitude
  cos(result) = cos(input).mul(itsAmplitude);
  sin(result) = sin(input).mul(itsAmplitude);
  // Filter the result with gaussian
  cv::sepFilter2D(cos(result), cos(result), -1, kernel, kernel,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
  cv::sepFilter2D(sin(result), sin(result), -1, kernel, kernel,
                  cv::Point(-1, -1), 0, cv::BORDER_REFLECT_101);
}

/////////////////
// Riesz Pyr  //
////////////////
RieszPyramid::RieszPyramid() {
  // Init low and highpass filter for pyramid construction/collapse
  this->lowPassFilter =
      (cv::Mat_<float>(9, 9) << -0.0001, -0.0007, -0.0023, -0.0046, -0.0057,
       -0.0046, -0.0023, -0.0007, -0.0001, -0.0007, -0.0030, -0.0047, -0.0025,
       -0.0003, -0.0025, -0.0047, -0.0030, -0.0007, -0.0023, -0.0047, 0.0054,
       0.0272, 0.0387, 0.0272, 0.0054, -0.0047, -0.0023, -0.0046, -0.0025,
       0.0272, 0.0706, 0.0910, 0.0706, 0.0272, -0.0025, -0.0046, -0.0057,
       -0.0003, 0.0387, 0.0910, 0.1138, 0.0910, 0.0387, -0.0003, -0.0057,
       -0.0046, -0.0025, 0.0272, 0.0706, 0.0910, 0.0706, 0.0272, -0.0025,
       -0.0046, -0.0023, -0.0047, 0.0054, 0.0272, 0.0387, 0.0272, 0.0054,
       -0.0047, -0.0023, -0.0007, -0.0030, -0.0047, -0.0025, -0.0003, -0.0025,
       -0.0047, -0.0030, -0.0007, -0.0001, -0.0007, -0.0023, -0.0046, -0.0057,
       -0.0046, -0.0023, -0.0007, -0.0001);

  this->highPassFilter =
      (cv::Mat_<float>(9, 9) << 0.0000, 0.0003, 0.0011, 0.0022, 0.0027, 0.0022,
       0.0011, 0.0003, 0.0000, 0.0003, 0.0020, 0.0059, 0.0103, 0.0123, 0.0103,
       0.0059, 0.0020, 0.0003, 0.0011, 0.0059, 0.0151, 0.0249, 0.0292, 0.0249,
       0.0151, 0.0059, 0.0011, 0.0022, 0.0103, 0.0249, 0.0402, 0.0469, 0.0402,
       0.0249, 0.0103, 0.0022, 0.0027, 0.0123, 0.0292, 0.0469, -0.9455, 0.0469,
       0.0292, 0.0123, 0.0027, 0.0022, 0.0103, 0.0249, 0.0402, 0.0469, 0.0402,
       0.0249, 0.0103, 0.0022, 0.0011, 0.0059, 0.0151, 0.0249, 0.0292, 0.0249,
       0.0151, 0.0059, 0.0011, 0.0003, 0.0020, 0.0059, 0.0103, 0.0123, 0.0103,
       0.0059, 0.0020, 0.0003, 0.0000, 0.0003, 0.0011, 0.0022, 0.0027, 0.0022,
       0.0011, 0.0003, 0.0000);
}
RieszPyramid::~RieszPyramid() {}
RieszPyramid::RieszPyramid(const RieszPyramid &other) {
  this->numLevels = other.numLevels;
  this->pyrLevels.resize(other.pyrLevels.size());
  other.lowPassFilter.copyTo(this->lowPassFilter);
  other.highPassFilter.copyTo(this->highPassFilter);
  for (int i = 0; i < this->numLevels; ++i) {
    this->pyrLevels[i] = other.pyrLevels[i];
  }
}
RieszPyramid &RieszPyramid::operator=(const RieszPyramid &other) {
  if (this != &other) {
    this->numLevels = other.numLevels;
    this->pyrLevels.resize(other.pyrLevels.size());
    other.lowPassFilter.copyTo(this->lowPassFilter);
    other.highPassFilter.copyTo(this->highPassFilter);
    for (int i = 0; i < this->numLevels; ++i) {
      this->pyrLevels[i] = other.pyrLevels[i];
    }
  }

  return *this;
}

void RieszPyramid::init(cv::Mat &frame, int levels) {
  this->pyrLevels.resize(levels);
  this->numLevels = levels;

  // Build pyramid from given frame
  buildPyramid(frame);

  // Init yet unused matrices
  for (int i = 0; i < this->numLevels; ++i) {
    RieszPyramidLevel &rpl = pyrLevels[i];
    const cv::Size size = rpl.itsLowpass.size();
    cos(rpl.itsRiesz) = cv::Mat::zeros(size, CV_32FC1);
    sin(rpl.itsRiesz) = cv::Mat::zeros(size, CV_32FC1);
    cos(rpl.itsPhaseDiff) = cv::Mat::zeros(size, CV_32FC1);
    sin(rpl.itsPhaseDiff) = cv::Mat::zeros(size, CV_32FC1);
    cos(rpl.itsLowpassIIR) = cv::Mat::zeros(size, CV_32FC1);
    sin(rpl.itsLowpassIIR) = cv::Mat::zeros(size, CV_32FC1);
    cos(rpl.itsHighpassIIR) = cv::Mat::zeros(size, CV_32FC1);
    sin(rpl.itsHighpassIIR) = cv::Mat::zeros(size, CV_32FC1);
    rpl.itsAmplitude = cv::Mat::zeros(size, CV_32FC1);
    rpl.itsAmplitudeBlurred = cv::Mat::zeros(size, CV_32FC1);

    sin(rpl.itsDenoisedOrients) = cv::Mat::zeros(size, CV_32FC1);
    cos(rpl.itsDenoisedOrients) = cv::Mat::zeros(size, CV_32FC1);
  }
}

// This builds a Riesz pyramid
void RieszPyramid::buildPyramid(const cv::Mat &frame) {
  const int max = this->numLevels - 1;

  if (max == -1)
    return;

  cv::Mat octave = frame;

  for (int i = 0; i < max; ++i) {
    cv::Mat hp, lp, down;

    // Highpass undergoes riesz transform
    cv::filter2D(octave, hp, CV_32FC1, highPassFilter, cv::Point(-1, -1), 0,
                 cv::BORDER_REFLECT_101);
    pyrLevels[i].build(hp, i);

    // Lowpass is passed onto the next level
    cv::filter2D(octave, lp, CV_32FC1, 2.0 * lowPassFilter, cv::Point(-1, -1),
                 0, cv::BORDER_REFLECT_101);
    octave = subsample(lp);
  }

  pyrLevels[max].build(octave, max);
}

void RieszPyramid::computePhaseDifferenceAndAmplitude(
    const RieszPyramid &prior) {
  const RieszPyramid::size_type max = pyrLevels.size() - 1;

  for (RieszPyramid::size_type i = 0; i < max; ++i) {
    pyrLevels[i].computePhaseDifferenceAndAmplitude(prior.pyrLevels[i]);
  }
}

// Amplify motion by alpha up to threshold using filtered phase data.
void RieszPyramid::amplify(double alpha, double threshold) {
  for (int i = this->numLevels - 2; i >= 0; i--) {
    pyrLevels[i].amplify(alpha, threshold);
  }
}

const cv::Mat RieszPyramid::subsample(cv::Mat &img) {
  // accept only grayscale float type matrices
  CV_Assert(img.depth() == CV_32FC1);
  CV_Assert(img.channels() == 1);
  CV_Assert(img.isContinuous());

  int nRowsBig = img.rows;
  int nColsBig = img.cols;

  cv::Mat tmp = cv::Mat::zeros(img.rows / 2 + (img.rows % 2),
                               img.cols / 2 + (img.cols % 2), CV_32FC1);

  float *p = img.ptr<float>(0);
  float *tmp_p = tmp.ptr<float>(0);

  for (int y = 0; y < nRowsBig; y += 2) {
    for (int x = 0; x < nColsBig; x += 2) {
      int subIdx = x / 2 + (y / 2) * tmp.cols;
      int bigIdx = x + y * nColsBig;

      tmp_p[subIdx] = p[bigIdx];
    }
  }

  return tmp.clone();
}

const cv::Mat RieszPyramid::injectZerosEven(cv::Mat &img) {
  // accept only grayscale float type matrices
  CV_Assert(img.depth() == CV_32FC1);
  CV_Assert(img.channels() == 1);
  CV_Assert(img.isContinuous());

  int nRows = img.rows;
  int nCols = img.cols;

  cv::Mat tmp = cv::Mat::zeros(nRows, nCols, CV_32FC1);

  float *p = img.ptr<float>(0);
  float *tmp_p = tmp.ptr<float>(0);

  for (int y = 0; y < nRows; y += 2) {
    for (int x = 0; x < nCols; x += 2) {

      int idx = x + y * nCols;
      tmp_p[idx] = p[idx];
    }
  }

  return tmp.clone();
}

// Return the frame resulting from the collapse of this pyramid.
//
const cv::Mat RieszPyramid::collapsePyramid() {
  const int count = pyrLevels.size() - 1;
  cv::Mat result = pyrLevels[count].itsLowpass;

  for (int i = count - 1; i >= 0; --i) {
    const cv::Mat &octave = pyrLevels[i].itsLowpass;
    cv::Mat lp, hp, up, up_zero;

    // Upsample with image without interpolation (= inject zeros on 3 of 4
    // pixels in every 4x4 neighborhood) Filter with lowpass after upsampling
    // (2.0*lpFilter) to make up for energy lost during upsampling
    cv::resize(result, up, octave.size(), 0, 0, cv::INTER_NEAREST);
    up_zero = injectZerosEven(up);
    cv::filter2D(up_zero, lp, CV_32FC1, 2.0 * lowPassFilter, cv::Point(-1, -1),
                 0, cv::BORDER_REFLECT_101);

    // Highpass on current levels img
    cv::filter2D(octave, hp, CV_32FC1, highPassFilter, cv::Point(-1, -1), 0,
                 cv::BORDER_REFLECT_101);

    // Reconstruct image adding LP and HP
    result = lp + hp;
  }
  return result;
}

cv::Size RieszPyramid::getLvlSize(int lvl) {
  return this->pyrLevels[lvl].itsSize;
}

std::vector<std::pair<int, int>> RieszPyramid::getSizes() {
  std::vector<std::pair<int, int>> result;

  for (int lvl = 0; lvl < this->numLevels; ++lvl) {
    cv::Size s = getLvlSize(lvl);
    auto tmp = std::pair<int, int>(s.height, s.width);
    result.push_back(tmp);
  }

  return result;
}

void RieszPyramid::denoisePhase() {
  const RieszPyramid::size_type max = pyrLevels.size() - 1;

  for (RieszPyramid::size_type i = 0; i < max; ++i) {
    pyrLevels[i].denoise(pyrLevels[i].itsPhaseDiff,
                         pyrLevels[i].itsDenoisedOrients);
  }
}