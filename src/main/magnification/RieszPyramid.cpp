///
// From https://github.com/tbl3rd/Pyramids
///
#include "RieszPyramid.h"

/////////////////////
// Riesz Pyr Level //
/////////////////////
RieszPyramidLevel::RieszPyramidLevel() { }
RieszPyramidLevel::~RieszPyramidLevel() { }
RieszPyramidLevel::RieszPyramidLevel(const RieszPyramidLevel& other)
{
           other.itsLp      .copyTo(        itsLp   );
    real ( other.itsR     ) .copyTo( real ( itsR     ));
    imag ( other.itsR     ) .copyTo( imag ( itsR     ));
    cos  ( other.itsPhase ) .copyTo( cos  ( itsPhase ));
    sin  ( other.itsPhase ) .copyTo( sin  ( itsPhase ));
}
RieszPyramidLevel& RieszPyramidLevel::operator=(const RieszPyramidLevel& other)
{
    if(this != &other)
    {
                   other.itsLp      .copyTo(        itsLp   );
            real ( other.itsR     ) .copyTo( real ( itsR     ));
            imag ( other.itsR     ) .copyTo( imag ( itsR     ));
            cos  ( other.itsPhase ) .copyTo( cos  ( itsPhase ));
            sin  ( other.itsPhase ) .copyTo( sin  ( itsPhase ));
    }

    return *this;
}

// Octave is a laplace pyr level. This applies x and yKernel
void RieszPyramidLevel::build(const cv::Mat &octave) {
    // This is the Riesz Band Filter, sometimes defined as [-0.5, 0 , 0.5], [-0.2,-0.48, 0, 0.48,0.2], [[-0.12,0,0.12],[-0.34, 0, 0.34],[-0.12,0,0.12]]
    static const cv::Mat realK = (cv::Mat_<float>(1, 3) << -0.49, 0, 0.49);
    static const cv::Mat imagK = realK.t();
    itsLp = octave;
    cv::filter2D(itsLp, real(itsR), itsLp.depth(), realK, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
    cv::filter2D(itsLp, imag(itsR), itsLp.depth(), imagK, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
}

// Write into result the element-wise inverse cosine of X.
void RieszPyramidLevel::arcCosX(const cv::Mat &X, cv::Mat &result) {
    assert(X.isContinuous() && result.isContinuous());
    const float *const pX =      X.ptr<float>(0);
    float *const pResult  = result.ptr<float>(0);
    const int count = X.rows * X.cols;

    for (int i = 0; i < count; ++i)
    {
        pResult[i] = acos(pX[i]);
    }
}

// This calculates movements separated by edges.
// Cos (itsPhase.first) are vertical edges
// Sin (itsPhase.second) are horizontal edges
void RieszPyramidLevel::unwrapOrientPhase(const RieszPyramidLevel &prior) {
    cv::Mat temp1
        =      itsLp.mul(prior.itsLp)
        + real(itsR).mul(real(prior.itsR))
        + imag(itsR).mul(imag(prior.itsR));
    cv::Mat temp2
        =       real(itsR).mul(prior.itsLp)
        - real(prior.itsR).mul(itsLp);
    cv::Mat temp3
        =       imag(itsR).mul(prior.itsLp)
        - imag(prior.itsR).mul(itsLp);
    cv::Mat tempP  = temp2.mul(temp2) + temp3.mul(temp3);
    cv::Mat phi    = tempP            + temp1.mul(temp1);
    cv::sqrt(phi, phi);
    cv::divide(temp1, phi, temp1);
    cv::patchNaNs(temp1, 0.0);
    arcCosX(temp1, phi);
    cv::sqrt(tempP, tempP);
    cv::divide(temp2, tempP, temp2);
    cv::patchNaNs(temp2, 0.0);
    cv::divide(temp3, tempP, temp3);
    cv::patchNaNs(temp3, 0.0);
    cos(itsPhase) = temp2.mul(phi);
    sin(itsPhase) = temp3.mul(phi);
}

// Write into result the element-wise cosines and sines of X.
void RieszPyramidLevel::cosSinX(const cv::Mat &X, CompExpMat &result)
{
    assert(X.isContinuous());
    cos(result) = cv::Mat::zeros(X.size(), CV_32F);
    sin(result) = cv::Mat::zeros(X.size(), CV_32F);
    assert(cos(result).isContinuous() && sin(result).isContinuous());
    const float *const pX =           X.ptr<float>(0);
    float *const pCosX    = cos(result).ptr<float>(0);
    float *const pSinX    = sin(result).ptr<float>(0);
    const int count = X.rows * X.cols;
    for (int i = 0; i < count; ++i) {
        pCosX[i] = cos(pX[i]);
        pSinX[i] = sin(pX[i]);
    }
}

cv::Mat RieszPyramidLevel::rms() {
    cv::Mat result = square(itsR) + itsLp.mul(itsLp);
    cv::sqrt(result, result);
    return result;
}

// Normalize the phase change of this level into result.
void RieszPyramidLevel::normalize(CompExpMat &result) {
    static const double sigma = 3.0;
    static const int aperture = static_cast<int>(1.0 + 4.0 * sigma);
    static const cv::Mat kernel
        = cv::getGaussianKernel(aperture, sigma, CV_32F);
    cv::Mat amplitude = rms();
    const CompExpMat change = itsRealPass - itsImagPass;
    cos(result) = cos(change).mul(amplitude);
    sin(result) = sin(change).mul(amplitude);
    cv::sepFilter2D(cos(result), cos(result), -1, kernel, kernel, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
    cv::sepFilter2D(sin(result), sin(result), -1, kernel, kernel, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
    cv::Mat temp;
    cv::sepFilter2D(amplitude, temp, -1, kernel, kernel, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
    cv::divide(cos(result), temp, cos(result));
    cv::patchNaNs(cos(result), 0.0);
    cv::divide(sin(result), temp, sin(result));
    cv::patchNaNs(sin(result), 0.0);
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
    cosSinX(MagV2, phaseDiff);
    cv::Mat pair = real(itsR).mul(cos(temp)) + imag(itsR).mul(sin(temp));
    cv::divide(pair, MagV, pair);
    cv::patchNaNs(pair, 0.0);
    itsLp = itsLp.mul(cos(phaseDiff)) - pair.mul(sin(phaseDiff));
}


/////////////////
// Riesz Pyr  //
////////////////
RieszPyramid::RieszPyramid()
{
    // Init low and highpass filter for pyramid construction/collapse
    this->lowPassFilter = (cv::Mat_<float>(9,9)<< -0.0001,   -0.0007,  -0.0023,  -0.0046,  -0.0057,  -0.0046,  -0.0023,  -0.0007,  -0.0001,
                                            -0.0007,   -0.0030,  -0.0047,  -0.0025,  -0.0003,  -0.0025,  -0.0047,  -0.0030,  -0.0007,
                                            -0.0023,   -0.0047,   0.0054,   0.0272,   0.0387,   0.0272,   0.0054,  -0.0047,  -0.0023,
                                            -0.0046,   -0.0025,   0.0272,   0.0706,   0.0910,   0.0706,   0.0272,  -0.0025,  -0.0046,
                                            -0.0057,   -0.0003,   0.0387,   0.0910,   0.1138,   0.0910,   0.0387,  -0.0003,  -0.0057,
                                            -0.0046,   -0.0025,   0.0272,   0.0706,   0.0910,   0.0706,   0.0272,  -0.0025,  -0.0046,
                                            -0.0023,   -0.0047,   0.0054,   0.0272,   0.0387,   0.0272,   0.0054,  -0.0047,  -0.0023,
                                            -0.0007,   -0.0030,  -0.0047,  -0.0025,  -0.0003,  -0.0025,  -0.0047,  -0.0030,  -0.0007,
                                            -0.0001,   -0.0007,  -0.0023,  -0.0046,  -0.0057,  -0.0046,  -0.0023,  -0.0007,  -0.0001);

    this->highPassFilter= (cv::Mat_<float>(9,9)<<  0.0000,    0.0003,   0.0011,   0.0022,   0.0027,   0.0022,   0.0011,   0.0003,   0.0000,
                                             0.0003,    0.0020,   0.0059,   0.0103,   0.0123,   0.0103,   0.0059,   0.0020,   0.0003,
                                             0.0011,    0.0059,   0.0151,   0.0249,   0.0292,   0.0249,   0.0151,   0.0059,   0.0011,
                                             0.0022,    0.0103,   0.0249,   0.0402,   0.0469,   0.0402,   0.0249,   0.0103,   0.0022,
                                             0.0027,    0.0123,   0.0292,   0.0469,  -0.9455,   0.0469,   0.0292,   0.0123,   0.0027,
                                             0.0022,    0.0103,   0.0249,   0.0402,   0.0469,   0.0402,   0.0249,   0.0103,   0.0022,
                                             0.0011,    0.0059,   0.0151,   0.0249,   0.0292,   0.0249,   0.0151,   0.0059,   0.0011,
                                             0.0003,    0.0020,   0.0059,   0.0103,   0.0123,   0.0103,   0.0059,   0.0020,   0.0003,
                                             0.0000,    0.0003,   0.0011,   0.0022,   0.0027,   0.0022,   0.0011,   0.0003,   0.0000);
}
RieszPyramid::~RieszPyramid() { }
RieszPyramid::RieszPyramid(const RieszPyramid& other)
{
    this->numLevels = other.numLevels;
    this->pyrLevels.resize(other.pyrLevels.size());
    other.lowPassFilter.copyTo(this->lowPassFilter);
    other.highPassFilter.copyTo(this->highPassFilter);
    for (int i = 0; i < this->numLevels; ++i)
    {
        this->pyrLevels[i] = other.pyrLevels[i];
    }
}
RieszPyramid& RieszPyramid::operator=(const RieszPyramid& other)
{
    if(this != &other)
    {
        this->numLevels = other.numLevels;
        this->pyrLevels.resize(other.pyrLevels.size());
        other.lowPassFilter.copyTo(this->lowPassFilter);
        other.highPassFilter.copyTo(this->highPassFilter);
        for (int i = 0; i < this->numLevels; ++i)
        {
            this->pyrLevels[i] = other.pyrLevels[i];
        }
    }

    return *this;
}

void RieszPyramid::init(cv::Mat &frame, int levels)
{
    this->pyrLevels.resize(levels);
    numLevels = levels;
    // Build pyramid from given frame
    buildPyramid(frame);
    for (int i = 0; i < this->numLevels; ++i) {
        RieszPyramidLevel &rpl = pyrLevels[i];
        const cv::Size size = rpl.itsLp.size();
        cos(rpl.itsPhase)    = cv::Mat::zeros(size, CV_32F);
        sin(rpl.itsPhase)    = cv::Mat::zeros(size, CV_32F);
        cos(rpl.itsRealPass) = cv::Mat::zeros(size, CV_32F);
        sin(rpl.itsRealPass) = cv::Mat::zeros(size, CV_32F);
        cos(rpl.itsImagPass) = cv::Mat::zeros(size, CV_32F);
        sin(rpl.itsImagPass) = cv::Mat::zeros(size, CV_32F);
    }
}

// This builds a Riesz pyramid
void RieszPyramid::buildPyramid(const cv::Mat &frame) {
    const int max = this->numLevels-1;
    cv::Mat octave = frame;

    for (int i = 0; i < max; ++i) {
        cv::Mat hp, lp, down;

        // Highpass undergoes riesz transform
        cv::filter2D(octave, hp, CV_32F, highPassFilter, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
        pyrLevels[i].build(hp);

        // Lowpass is passed onto the next level
        cv::filter2D(octave, lp, CV_32F, 2.0*lowPassFilter, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);
        octave = subsample(lp);
    }

    pyrLevels[max].build(octave);
}

void RieszPyramid::unwrapOrientPhase(const RieszPyramid &prior) {
    const RieszPyramid::size_type max = pyrLevels.size() - 1;

    for (RieszPyramid::size_type i = 0; i < max; ++i)
    {
        pyrLevels[i].unwrapOrientPhase(prior.pyrLevels[i]);
    }
}

// Amplify motion by alpha up to threshold using filtered phase data.
void RieszPyramid::amplify(double alpha, double threshold)
{
    for(int i = this->numLevels-1; i >= 0; i--) {
        pyrLevels[i].amplify(alpha, threshold);
    }
}

const cv::Mat RieszPyramid::subsample(cv::Mat &img) {
    // accept only grayscale float type matrices
    CV_Assert(img.depth() == CV_32F);
    CV_Assert(img.channels() == 1);
    CV_Assert(img.isContinuous());

    int nRowsBig = img.rows;
    int nColsBig = img.cols;

    cv::Mat tmp = cv::Mat::zeros(img.rows/2 + (img.rows%2), img.cols/2 + (img.cols%2), CV_32F);

    float *p = img.ptr<float>(0);
    float *tmp_p = tmp.ptr<float>(0);

    for (int y = 0; y < nRowsBig; y += 2) {
        for (int x = 0; x < nColsBig; x += 2) {
            int subIdx = x/2 + (y/2)*tmp.cols;
            int bigIdx = x + y*nColsBig;

            tmp_p[subIdx] = p[bigIdx];
        }
    }

    return tmp.clone();
}

const cv::Mat RieszPyramid::injectZerosEven(cv::Mat &img) {
    // accept only grayscale float type matrices
    CV_Assert(img.depth() == CV_32F);
    CV_Assert(img.channels() == 1);
    CV_Assert(img.isContinuous());

    int nRows = img.rows;
    int nCols = img.cols;

    cv::Mat tmp = cv::Mat::zeros(nRows, nCols, CV_32F);

    float *p = img.ptr<float>(0);
    float *tmp_p = tmp.ptr<float>(0);

    for (int y = 0; y < nRows; y += 2) {
        for (int x = 0; x < nCols; x += 2) {

            int idx = x + y*nCols;
            tmp_p[idx] = p[idx];
        }
    }

    return tmp.clone();
}

// Return the frame resulting from the collapse of this pyramid.
//
const cv::Mat RieszPyramid::collapsePyramid() {
    const int count = pyrLevels.size() - 1;
    cv::Mat result = pyrLevels[count].itsLp;

    for (int i = count - 1; i >= 0; --i) {
       const cv::Mat &octave = pyrLevels[i].itsLp;
        cv::Mat lp, hp, up, up_zero;

        // Upsample with image without interpolation (= inject zeros on 3 of 4 pixels in every 4x4 neighborhood)
        // Filter with lowpass after upsampling (2.0*lpFilter) to make up for energy lost during upsampling
        cv::resize(result, up, octave.size(), 0, 0, cv::INTER_NEAREST);
        up_zero = injectZerosEven(up);
        cv::filter2D(up_zero, lp, CV_32F, 2.0*lowPassFilter, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);

        // Highpass on current levels img
        cv::filter2D(octave, hp, CV_32F, highPassFilter, cv::Point(-1,-1), 0, cv::BORDER_REFLECT_101);

        // Reconstruct image adding LP and HP
        result = lp + hp;
    }
    return result;
}
