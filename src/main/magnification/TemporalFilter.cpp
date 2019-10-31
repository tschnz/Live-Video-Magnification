/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->TemporalFilter.cpp                                 */
/*                                                                                  */
/* This program is free software: you can redistribute it and/or modify             */
/* it under the terms of the GNU General Public License as published by             */
/* the Free Software Foundation, either version 3 of the License, or                */
/* (at your option) any later version.                                              */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful,                  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    */
/* GNU General Public License for more details.                                     */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.            */
/************************************************************************************/

#include "main/magnification/TemporalFilter.h"

////////////////////////
///Filter //////////////
////////////////////////
void iirFilter(const Mat &src, Mat &dst, Mat &lowpassHi, Mat &lowpassLo,
               double cutoffLo, double cutoffHi)
{
    // Set minimum for cutoff, so low cutoff gets faded out
    if(cutoffLo == 0)
        cutoffLo = 0.01;

    /* The higher cutoff*, the faster the lowpass* image of the lowpass* pyramid gets faded out.
     * That means, a high cutoff weights new images (= \param src)
     * more than the old ones (= \param lowpass*), so long lasting movements are faded out fast.
     * The other way, a low cutoff evens out fast movements ocurring only in a few number of src images. */

    Mat tmp1 = (1-cutoffHi)*lowpassHi + cutoffHi*src;
    Mat tmp2 = (1-cutoffLo)*lowpassLo + cutoffLo*src;
    lowpassHi = tmp1;
    lowpassLo = tmp2;

    dst = lowpassHi - lowpassLo;
}

void iirWaveletFilter(const vector<Mat> &src, vector<Mat> &dst, vector<Mat> &lowpassHi, vector<Mat> &lowpassLo,
                      double cutoffLo, double cutoffHi)
{
    // Set minimum for cutoff, so low cutoff gets faded out
    if(cutoffLo == 0)
        cutoffLo = 0.01;

    /* The higher cutoff*, the faster the lowpass* image of the lowpass* pyramid gets faded out.
     * That means, a high cutoff weights new images (= \param src)
     * more than the old ones (= \param lowpass*), so long lasting movements are faded out fast.
     * The other way, a low cutoff evens out fast movements ocurring only in a few number of src images. */

    // Do this for every detail/coefficient image
    for(int dims = 0; dims < 3; dims++) {
        Mat tmp1 = (1-cutoffHi)*lowpassHi[dims] + cutoffHi*src[dims];
        Mat tmp2 = (1-cutoffLo)*lowpassLo[dims] + cutoffLo*src[dims];
        lowpassHi[dims] = tmp1;
        lowpassLo[dims] = tmp2;

        dst[dims] = lowpassHi[dims] - lowpassLo[dims];
    }
}

void idealFilter(const Mat &src, Mat &dst , double cutoffLo, double cutoffHi, double framerate)
{
    if(cutoffLo == 0.00)
        cutoffLo += 0.01;

    int channelNrs = src.channels();
    Mat *channels = new Mat[channelNrs];
    split(src, channels);

    // Apply filter on each channel individually
    for (int curChannel = 0; curChannel < channelNrs; ++curChannel) {
        Mat current = channels[curChannel];
        Mat tempImg;

        int width = current.cols;
        int height = getOptimalDFTSize(current.rows);

        copyMakeBorder(current, tempImg,
                       0, height - current.rows,
                       0, width - current.cols,
                       BORDER_CONSTANT, Scalar::all(0));

        // DFT
        dft(tempImg, tempImg, DFT_ROWS | DFT_SCALE);

        // construct Filter
        Mat filter = tempImg.clone();
        createIdealBandpassFilter(filter, cutoffLo, cutoffHi, framerate);

        // apply
        mulSpectrums(tempImg, filter, tempImg, DFT_ROWS);

        // inverse
        idft(tempImg, tempImg, DFT_ROWS | DFT_SCALE);

        tempImg(Rect(0, 0, current.cols, current.rows)).copyTo(channels[curChannel]);
    }
    merge(channels, channelNrs, dst);

    normalize(dst, dst, 0, 1, cv::NORM_MINMAX);
    delete [] channels;
}

void createIdealBandpassFilter(Mat &filter, double cutoffLo, double cutoffHi, double framerate)
{
    float width = filter.cols;
    float height = filter.rows;

    // Calculate frequencies according to framerate and size
    double fl = 2 * cutoffLo * width / framerate;
    double fh = 2 * cutoffHi * width / framerate;

    double response;

    // Create the filtermask, looks like the quarter of a circle
    for(int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if(x >= fl && x <= fh)
                response = 1.0f;
            else
                response = 0.0f;

            filter.at<float>(y,x) = response;
        }
    }
}

////////////////////////
///Helper //////////////
////////////////////////
void img2tempMat(const Mat &frame, Mat &dst, int maxImages)
{
    // Reshape in 1 column
    Mat reshaped = frame.reshape(frame.channels(), frame.cols*frame.rows).clone();

    if(frame.channels() == 1)
        reshaped.convertTo(reshaped, CV_32FC1);
    else
        reshaped.convertTo(reshaped, CV_32FC3);

    // First frame
    if(dst.cols == 0) {
        reshaped.copyTo(dst);
    }
    // Later frames
    else {
        hconcat(dst,reshaped,dst);
    }

    // If dst reaches maximum, delete the first column (eg the oldest image)
    if(dst.cols > maxImages && maxImages != 0) {
        dst.colRange(1,dst.cols).copyTo(dst);
    }
}

void tempMat2img(const Mat &src, int position, const Size &frameSize, Mat &frame)
{
    Mat line = src.col(position).clone();
    frame = line.reshape(line.channels(), frameSize.height).clone();
}

////////////////////////
///Butterworth /////////
////////////////////////
///
// From https://github.com/tbl3rd/Pyramids
///

// An ordering on complex that lets real roots dominate.
static bool sortComplex(std::complex<double> x, std::complex<double> y)
{
    if (std::real(x) < std::real(y)) return true;
    if (std::real(y) < std::real(x)) return false;
    return std::imag(x) < std::imag(y);
}
static bool hasPosImag(const std::complex<double> &z)
{
    return std::imag(z) > 0;
}
static bool hasNegImag(const std::complex<double> &z)
{
    return std::imag(z) < 0;
}

// Return the polynomial coefficients for roots.
// Keep roots sorted with sortComplex() to maintain precision.
//
static std::vector< std::complex<double> >
        polynomialCoefficients(std::vector< std::complex<double> > roots)
{
    std::vector< std::complex<double> > coeffs(roots.size() + 1, 0.0);
    coeffs[0] = 1.0;
    std::sort(roots.begin(), roots.end(), sortComplex);
    unsigned int sofar = 1;
    for (unsigned int k = 0; k < roots.size(); ++k) {
        const std::complex<double> w = -roots[k];
        for (unsigned int j = sofar; j > 0; --j) {
            coeffs[j] = coeffs[j] * w + coeffs[j - 1];
        }
        coeffs[0] *= w;
        ++sofar;
    }
    std::vector< std::complex<double> > result = coeffs;

    std::vector< std::complex<double> > pos_roots = roots;
    std::vector< std::complex<double> >::iterator pos_end;
    pos_end = std::remove_if(pos_roots.begin(), pos_roots.end(), hasNegImag);
    pos_roots.assign(pos_roots.begin(), pos_end);
    std::sort(pos_roots.begin(), pos_roots.end(), sortComplex);

    std::vector< std::complex<double> > neg_roots = roots;
    std::vector< std::complex<double> >::iterator neg_end;
    neg_end = std::remove_if(neg_roots.begin(), neg_roots.end(), hasPosImag);
    neg_roots.assign(neg_roots.begin(), neg_end);
    std::sort(neg_roots.begin(), neg_roots.end(), sortComplex);

    const bool same
        =  neg_roots.size() == pos_roots.size()
        && std::equal(pos_roots.begin(), pos_roots.end(), neg_roots.begin());
    if (same) {
        for (unsigned k = 0; k < coeffs.size(); ++k) {
            result[k] = std::real(coeffs[k]);
        }
    }
    return result;
}

// Write into a and b the real polynomial transfer function coefficients
// from gain, zeros, and poles.
//
static void zerosPolesToTransferCoefficients(std::vector< std::complex<double> > zeros,
                                 std::vector< std::complex<double> > poles,
                                 double gain,
                                 std::vector< std::complex<double> > &a,
                                 std::vector< std::complex<double> > &b)
{
    a = polynomialCoefficients(poles);
    b = polynomialCoefficients(zeros);
    for (unsigned int k = 0; k < b.size(); ++k) b[k] *= gain;
}


// Normalize the polynomial representation of the real transfer
// coefficients in a and b.
//
// Output coefficients will also be real numbers.
// Remove leading zeros to avoid dividing by 0.
//
static void normalize(std::vector< std::complex<double> > &b,
                      std::vector< std::complex<double> > &a)
{
    while (a.front() == 0.0 && a.size() > 1) a.erase(a.begin());
    const std::complex<double> leading_coeff = a.front();
    for (unsigned int k = 0; k < a.size(); ++k) a[k] /= leading_coeff;
    for (unsigned int k = 0; k < b.size(); ++k) b[k] /= leading_coeff;
}


// Return the binomial coefficient: n choose k.
static unsigned choose(unsigned n, unsigned k)
{
    if (k > n) return 0;
    if (k * 2 > n) k = n - k;
    if (k == 0) return 1;
    unsigned result = n;
    for (unsigned i = 2; i <= k; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}


// Use the bilinear transform to convert the analog filter coefficients in
// a and b into a digital filter for the sampling frequency fs (1/T).
static void bilinearTransform(std::vector< std::complex<double> > &b,
                              std::vector< std::complex<double> > &a,
                              double fs)
{
    const unsigned int D = a.size() - 1;
    const unsigned int N = b.size() - 1;
    const unsigned int M = std::max(N, D);
    const unsigned int Np = M;
    const unsigned int Dp = M;

    std::vector< std::complex<double> > bprime(Np + 1, 0.0);
    for (unsigned j = 0; j < Np + 1; ++j) {
        std::complex<double> val = 0.0;
        for (unsigned i = 0; i < N + 1; ++i) {
            for (unsigned k = 0; k < i + 1; ++k) {
                for (unsigned l = 0; l < M - i + 1; ++l) {
                    if (k + l == j) {
                        val += std::complex<double>(choose(i, k))
                            *  std::complex<double>(choose(M - i, l))
                            *  b[N - i] * pow(2.0 * fs, i) * pow(-1.0, k);
                    }
                }
            }
        }
        bprime[j] = real(val);
    }

    std::vector< std::complex<double> > aprime(Dp + 1, 0.0);
    for (unsigned j = 0; j < Dp + 1; ++j) {
        std::complex<double> val = 0.0;
        for (unsigned i = 0; i < D + 1; ++i) {
            for(unsigned k = 0; k < i + 1; ++k) {
                for(unsigned l = 0; l < M - i + 1; ++l) {
                    if (k + l == j) {
                        val += std::complex<double>(choose(i, k))
                            *  std::complex<double>(choose(M - i, l))
                            *  a[D - i] * pow(2.0 * fs, i) * pow(-1.0, k);
                    }
                }
            }
        }
        aprime[j] = real(val);
    }

    normalize(bprime, aprime);
    a = aprime;
    b = bprime;
}


// Transform a and b coefficients of transfer function
// into a low-pass filter with cutoff frequency w_0.
// Assume the transfer function has only real coefficients.
static void toLowpass(std::vector< std::complex<double> > &b,
                      std::vector< std::complex<double> > &a,
                      double w0)
{
    std::vector<double> pwo;
    const int d = a.size();
    const int n = b.size();
    const int M = int(std::max(double(d), double(n)));
    const unsigned int start1 = int(std::max(double(n - d), 0.0));
    const unsigned int start2 = int(std::max(double(d - n), 0.0));
    for (int k = M - 1; k > -1; --k) pwo.push_back(pow(w0, double(k)));
    unsigned int k;
    for (k = start2; k < pwo.size() && k - start2 < b.size(); ++k) {
        b[k - start2]
            *= std::complex<double>(pwo[start1])
            /  std::complex<double>(pwo[k]);
        if( std::isnan(std::abs(b[k - start2])) || std::isinf(std::abs(b[k - start2])) ) {
            b[k - start2] = std::complex<double>(0.0);
        }
    }

    for (k = start1; k < pwo.size() && k - start1 < a.size(); ++k) {
        a[k - start1]
            *= std::complex<double>(pwo[start1])
            /  std::complex<double>(pwo[k]);

        if( std::isnan(std::abs(a[k - start1])) || std::isinf(std::abs(a[k - start1])) ) {
            a[k - start1] = std::complex<double>(0.0);
        }
    }

    normalize(b, a);
}


// Compute zeros, poles and gain for filter of order N assuming the
// normalized Butterworth form of transfer function.
//
// The gain is always 1.0, but parameterized to agree with textbooks.
static void prototypeAnalogButterworth(unsigned N,
                           std::vector< std::complex<double> > &zeros,
                           std::vector< std::complex<double> > &poles,
                           double &gain)
{
    static const std::complex<double> j = std::complex<double>(0, 1.0);
    for (unsigned k = 1; k < N + 1; ++k) {
        poles.push_back(exp(j * (2.0 * k - 1) / (2.0 * N) * M_PI) * j);
    }
    gain = 1.0;
    zeros.clear();
}


// Tangentially warp the Wn input analog frequency to W0
// for the resulting digital filter.
//
// W0 is the warped (high or low) bandpass cutoff frequency.
//
// See http://www.robots.ox.ac.uk/~sjrob/Teaching/SP/l6.pdf
void butterworth(unsigned int N, double Wn,
                 std::vector<double> &out_a,
                 std::vector<double> &out_b)
{
    static const double fs = 2.0;
    const double w0 = 2.0 * fs * tan(M_PI * Wn / fs);
    std::vector< std::complex<double> > zeros, poles;
    double gain;
    prototypeAnalogButterworth(N, zeros, poles, gain);
    std::vector< std::complex<double> > a, b;
    zerosPolesToTransferCoefficients(zeros, poles, gain, a, b);
    toLowpass(b, a, w0);
    bilinearTransform(b, a, fs);
    out_a.clear();
    for (unsigned k = 0; k < a.size(); ++k) out_a.push_back(std::real(a[k]));
    out_b.clear();
    for (unsigned k = 0; k < b.size(); ++k) out_b.push_back(std::real(b[k]));
}

//////////////////////////////////////////////////
// Riesz Transform Butterworth Bandpass Filter //
/////////////////////////////////////////////////
void RieszTemporalFilter::updateFrequency(double f)
{
    this->itsFrequency = f;
    this->computeCoefficients();
}

void RieszTemporalFilter::updateFramerate(double framerate)
{
    this->itsFramerate = framerate;
    this->computeCoefficients();
}
void RieszTemporalFilter::computeCoefficients()
{
    const double Wn = itsFrequency / (itsFramerate/2.0);
    butterworth(1, Wn, itsA, itsB);
}
void RieszTemporalFilter::passEach(cv::Mat &result,
              const cv::Mat &phase,
              const cv::Mat &prior) {
    result
        = itsB[0] * phase
        + itsB[1] * prior
        - itsA[1] * result;
    result /= itsA[0];
    cv::patchNaNs(result, 0.0);
}
void RieszTemporalFilter::pass(CompExpMat &result,
          const CompExpMat &phase,
          const CompExpMat &prior) {
    passEach(cos(result), cos(phase), cos(prior));
    passEach(sin(result), sin(phase), sin(prior));
}
