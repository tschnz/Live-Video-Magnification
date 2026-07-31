#include "processing/magnification/TemporalFilter.hpp"

#include <algorithm>
#include <cmath>
#include <complex>

namespace livim {

void iirFilter(const cv::Mat& src, cv::Mat& dst, cv::Mat& lowpassHi, cv::Mat& lowpassLo,
               double cutoffLo, double cutoffHi) {
    if (cutoffLo == 0)
        cutoffLo = 0.01;

    // A high cutoff weights new images over the retained lowpass, so long-lasting movements fade
    // out fast; a low cutoff instead evens out movements spanning only a few frames.
    cv::Mat tmp1 = (1 - cutoffHi) * lowpassHi + cutoffHi * src;
    cv::Mat tmp2 = (1 - cutoffLo) * lowpassLo + cutoffLo * src;
    lowpassHi = tmp1;
    lowpassLo = tmp2;

    dst = lowpassHi - lowpassLo;
}

void idealFilter(const cv::Mat& src, cv::Mat& dst, double cutoffLo, double cutoffHi,
                 double framerate) {
    if (cutoffLo == 0.00)
        cutoffLo += 0.01;

    int channelNrs = src.channels();
    cv::Mat* channels = new cv::Mat[channelNrs];
    cv::split(src, channels);

    for (int curChannel = 0; curChannel < channelNrs; ++curChannel) {
        cv::Mat current = channels[curChannel];
        cv::Mat tempImg;

        int width = current.cols;
        int height = cv::getOptimalDFTSize(current.rows);

        cv::copyMakeBorder(current, tempImg, 0, height - current.rows, 0, width - current.cols,
                           cv::BORDER_CONSTANT, cv::Scalar::all(0));

        cv::dft(tempImg, tempImg, cv::DFT_ROWS | cv::DFT_SCALE);

        cv::Mat filter = tempImg.clone();
        createIdealBandpassFilter(filter, cutoffLo, cutoffHi, framerate);

        cv::mulSpectrums(tempImg, filter, tempImg, cv::DFT_ROWS);
        cv::idft(tempImg, tempImg, cv::DFT_ROWS | cv::DFT_SCALE);

        tempImg(cv::Rect(0, 0, current.cols, current.rows)).copyTo(channels[curChannel]);
    }
    cv::merge(channels, channelNrs, dst);

    cv::normalize(dst, dst, 0, 1, cv::NORM_MINMAX);
    delete[] channels;
}

void createIdealBandpassFilter(cv::Mat& filter, double cutoffLo, double cutoffHi,
                               double framerate) {
    float width = filter.cols;
    float height = filter.rows;

    // Hz -> DFT bin index.
    double fl = 2 * cutoffLo * width / framerate;
    double fh = 2 * cutoffHi * width / framerate;

    double response;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x >= fl && x <= fh)
                response = 1.0f;
            else
                response = 0.0f;

            filter.at<float>(y, x) = response;
        }
    }
}

int getOptimalBufferSize(int fps) {
    // Two seconds of footage, rounded up to a power of two.
    unsigned int round = (unsigned int)std::max(2 * fps, 16);
    round--;
    round |= round >> 1;
    round |= round >> 2;
    round |= round >> 4;
    round |= round >> 8;
    round |= round >> 16;
    round++;

    return round;
}

// An ordering on complex that lets real roots dominate.
static bool sortComplex(std::complex<double> x, std::complex<double> y) {
    if (std::real(x) < std::real(y))
        return true;
    if (std::real(y) < std::real(x))
        return false;
    return std::imag(x) < std::imag(y);
}
static bool hasPosImag(const std::complex<double>& z) { return std::imag(z) > 0; }
static bool hasNegImag(const std::complex<double>& z) { return std::imag(z) < 0; }

// Polynomial coefficients for `roots`, kept sorted with sortComplex() to maintain precision.
static std::vector<std::complex<double>>
polynomialCoefficients(std::vector<std::complex<double>> roots) {
    std::vector<std::complex<double>> coeffs(roots.size() + 1, 0.0);
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
    std::vector<std::complex<double>> result = coeffs;

    std::vector<std::complex<double>> pos_roots = roots;
    std::vector<std::complex<double>>::iterator pos_end;
    pos_end = std::remove_if(pos_roots.begin(), pos_roots.end(), hasNegImag);
    pos_roots.assign(pos_roots.begin(), pos_end);
    std::sort(pos_roots.begin(), pos_roots.end(), sortComplex);

    std::vector<std::complex<double>> neg_roots = roots;
    std::vector<std::complex<double>>::iterator neg_end;
    neg_end = std::remove_if(neg_roots.begin(), neg_roots.end(), hasPosImag);
    neg_roots.assign(neg_roots.begin(), neg_end);
    std::sort(neg_roots.begin(), neg_roots.end(), sortComplex);

    const bool same = neg_roots.size() == pos_roots.size() &&
                      std::equal(pos_roots.begin(), pos_roots.end(), neg_roots.begin());
    if (same) {
        for (unsigned k = 0; k < coeffs.size(); ++k) {
            result[k] = std::real(coeffs[k]);
        }
    }
    return result;
}

// Write into a and b the real polynomial transfer function coefficients from gain, zeros and poles.
static void zerosPolesToTransferCoefficients(std::vector<std::complex<double>> zeros,
                                             std::vector<std::complex<double>> poles, double gain,
                                             std::vector<std::complex<double>>& a,
                                             std::vector<std::complex<double>>& b) {
    a = polynomialCoefficients(poles);
    b = polynomialCoefficients(zeros);
    for (unsigned int k = 0; k < b.size(); ++k)
        b[k] *= gain;
}

// Normalize the polynomial representation of the real transfer coefficients in a and b.
static void normalize(std::vector<std::complex<double>>& b, std::vector<std::complex<double>>& a) {
    const std::complex<double> leading_coeff = a.front();

    for (unsigned int k = 0; k < a.size(); ++k)
        a[k] = (leading_coeff == 0.0) ? 0.0 : a[k] / leading_coeff;
    for (unsigned int k = 0; k < b.size(); ++k)
        b[k] = (leading_coeff == 0.0) ? 0.0 : b[k] / leading_coeff;
}

// Return the binomial coefficient: n choose k.
static unsigned choose(unsigned n, unsigned k) {
    if (k > n)
        return 0;
    if (k * 2 > n)
        k = n - k;
    if (k == 0)
        return 1;
    unsigned result = n;
    for (unsigned i = 2; i <= k; ++i) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}

// Use the bilinear transform to convert the analog filter coefficients in
// a and b into a digital filter for the sampling frequency fs (1/T).
static void bilinearTransform(std::vector<std::complex<double>>& b,
                              std::vector<std::complex<double>>& a, double fs) {
    const unsigned int D = a.size() - 1;
    const unsigned int N = b.size() - 1;
    const unsigned int M = std::max(N, D);
    const unsigned int Np = M;
    const unsigned int Dp = M;

    std::vector<std::complex<double>> bprime(Np + 1, 0.0);
    for (unsigned j = 0; j < Np + 1; ++j) {
        std::complex<double> val = 0.0;
        for (unsigned i = 0; i < N + 1; ++i) {
            for (unsigned k = 0; k < i + 1; ++k) {
                for (unsigned l = 0; l < M - i + 1; ++l) {
                    if (k + l == j) {
                        val += std::complex<double>(choose(i, k)) *
                               std::complex<double>(choose(M - i, l)) * b[N - i] *
                               pow(2.0 * fs, i) * pow(-1.0, k);
                    }
                }
            }
        }
        bprime[j] = real(val);
    }

    std::vector<std::complex<double>> aprime(Dp + 1, 0.0);
    for (unsigned j = 0; j < Dp + 1; ++j) {
        std::complex<double> val = 0.0;
        for (unsigned i = 0; i < D + 1; ++i) {
            for (unsigned k = 0; k < i + 1; ++k) {
                for (unsigned l = 0; l < M - i + 1; ++l) {
                    if (k + l == j) {
                        val += std::complex<double>(choose(i, k)) *
                               std::complex<double>(choose(M - i, l)) * a[D - i] *
                               pow(2.0 * fs, i) * pow(-1.0, k);
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

// Transform the a/b transfer-function coefficients into a low-pass filter with cutoff w0.
// Assumes the transfer function has only real coefficients.
static void toLowpass(std::vector<std::complex<double>>& b, std::vector<std::complex<double>>& a,
                      double w0) {
    std::vector<double> pwo;
    const int d = a.size();
    const int n = b.size();
    const int M = int(std::max(double(d), double(n)));
    const unsigned int start1 = int(std::max(double(n - d), 0.0));
    const unsigned int start2 = int(std::max(double(d - n), 0.0));
    for (int k = M - 1; k > -1; --k)
        pwo.push_back(pow(w0, double(k)));
    unsigned int k;
    for (k = start2; k < pwo.size() && k - start2 < b.size(); ++k) {
        if (pwo[k] == 0.0) {
            b[k - start2] = 0.0;
            continue;
        }

        b[k - start2] *= std::complex<double>(pwo[start1]) / std::complex<double>(pwo[k]);
    }

    for (k = start1; k < pwo.size() && k - start1 < a.size(); ++k) {
        if (pwo[k] == 0.0) {
            a[k - start1] = 0.0;
            continue;
        }

        a[k - start1] *= std::complex<double>(pwo[start1]) / std::complex<double>(pwo[k]);
    }

    normalize(b, a);
}

// Zeros, poles and gain for a filter of order N in normalized Butterworth form. The gain is
// always 1.0, but parameterized to agree with textbooks.
static void prototypeAnalogButterworth(unsigned N, std::vector<std::complex<double>>& zeros,
                                       std::vector<std::complex<double>>& poles, double& gain) {
    static const std::complex<double> j = std::complex<double>(0, 1.0);
    for (unsigned k = 1; k < N + 1; ++k) {
        poles.push_back(exp(j * (2.0 * k - 1) / (2.0 * N) * CV_PI) * j);
    }
    gain = 1.0;
    zeros.clear();
}

// Tangentially warps the Wn input analog frequency to the w0 bandpass cutoff of the resulting
// digital filter. See http://www.robots.ox.ac.uk/~sjrob/Teaching/SP/l6.pdf
void butterworth(unsigned int N, double Wn, std::vector<double>& out_a,
                 std::vector<double>& out_b) {
    static const double fs = 2.0;
    const double w0 = 2.0 * fs * tan(CV_PI * Wn / fs);
    std::vector<std::complex<double>> zeros, poles;
    double gain;
    prototypeAnalogButterworth(N, zeros, poles, gain);
    std::vector<std::complex<double>> a, b;
    zerosPolesToTransferCoefficients(zeros, poles, gain, a, b);
    toLowpass(b, a, w0);
    bilinearTransform(b, a, fs);
    out_a.clear();
    for (unsigned k = 0; k < a.size(); ++k)
        out_a.push_back(std::real(a[k]));
    out_b.clear();
    for (unsigned k = 0; k < b.size(); ++k)
        out_b.push_back(std::real(b[k]));
}

RieszTemporalFilter::RieszTemporalFilter(double frq, double fps,
                                         std::vector<std::pair<int, int>> lvlSizes)
    : itsFrequency(frq), itsFramerate(fps), itsA(), itsB(), numPyrLvls(lvlSizes.size()) {
    itsRegister0.resize(lvlSizes.size());
    itsRegister1.resize(lvlSizes.size());
    itsPhase.resize(lvlSizes.size());
    for (size_t lvl = 0; lvl < lvlSizes.size(); ++lvl) {
        sin(itsRegister0[lvl]) =
            cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
        cos(itsRegister0[lvl]) =
            cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
        sin(itsRegister1[lvl]) =
            cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
        cos(itsRegister1[lvl]) =
            cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
        sin(itsPhase[lvl]) = cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
        cos(itsPhase[lvl]) = cv::Mat::zeros(lvlSizes[lvl].first, lvlSizes[lvl].second, CV_32FC1);
    }
}

void RieszTemporalFilter::updateFrequency(double f) {
    this->itsFrequency = f;
    this->computeCoefficients();
}

void RieszTemporalFilter::computeCoefficients() {
    const double Wn = itsFramerate == 0.0 ? 0.0 : itsFrequency / (itsFramerate / 2.0);
    butterworth(2, Wn, itsA, itsB);
}
void RieszTemporalFilter::passEach(cv::Mat& result, const cv::Mat& phase, const cv::Mat& prior) {
    result = itsB[0] * phase + itsB[1] * prior - itsA[1] * result;
    result = (itsA[0] == 0) ? cv::Mat() : result / itsA[0];
}
void RieszTemporalFilter::pass(CompExpMat& result, const CompExpMat& phase,
                               const CompExpMat& prior) {
    passEach(cos(result), cos(phase), cos(prior));
    passEach(sin(result), sin(phase), sin(prior));
}

// Direct Form Type II (Oppenheim and Schafer 3rd Ed., pp. 388-390). Assumes B and A hold three
// coefficients each and that A(1) == 1.
void RieszTemporalFilter::IIRTemporalFilter(CompExpMat& result, const CompExpMat& phaseDiff,
                                            int lvl) {
    // Accumulating the quaternionic phase difference is equivalent to phase unwrapping.
    this->itsPhase[lvl] += phaseDiff;

    result = (this->itsPhase[lvl] * this->itsB[0]) + this->itsRegister0[lvl];

    this->itsRegister0[lvl] = (this->itsPhase[lvl] * this->itsB[1]) + this->itsRegister1[lvl] -
                              (result * this->itsA[1]);

    this->itsRegister1[lvl] = (this->itsPhase[lvl] * this->itsB[2]) - (result * this->itsA[2]);
}

void RieszTemporalFilter::resetMat() {
    for (size_t lvl = 0; lvl < numPyrLvls; ++lvl) {
        sin(itsRegister0[lvl]) = 0.f;
        cos(itsRegister0[lvl]) = 0.f;
        sin(itsRegister1[lvl]) = 0.f;
        cos(itsRegister1[lvl]) = 0.f;
        sin(itsPhase[lvl]) = 0.f;
        cos(itsPhase[lvl]) = 0.f;
    }
}

} // namespace livim
