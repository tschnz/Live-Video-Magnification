///
// From https://github.com/tbl3rd/Pyramids
///

#ifndef COMPLEXMAT_H
#define COMPLEXMAT_H

#include <opencv2/core/core.hpp>

// Because C++ forbids std::complex<cv::Mat> for who knows what reason ...
//
// ComplexMat and CompExpMat are just two suggestive names for the same
// type.  ComplexMat has real and imaginary parts.  ComplexExp has cosine
// and sine parts, which also happen to be real and imaginary parts.
//
typedef std::pair<cv::Mat, cv::Mat> ComplexMat; // a real and imaginary matrix
typedef std::pair<cv::Mat, cv::Mat> CompExpMat; // a cos and sin matrix

template<typename T> T  real(const std::pair<T, T> &p) { return p.first;  }
template<typename T> T &real(      std::pair<T, T> &p) { return p.first;  }
template<typename T> T  imag(const std::pair<T, T> &p) { return p.second; }
template<typename T> T &imag(      std::pair<T, T> &p) { return p.second; }
template<typename T> T   cos(const std::pair<T, T> &p) { return p.first;  }
template<typename T> T  &cos(      std::pair<T, T> &p) { return p.first;  }
template<typename T> T   sin(const std::pair<T, T> &p) { return p.second; }
template<typename T> T  &sin(      std::pair<T, T> &p) { return p.second; }

template<typename T> std::pair<T, T> &
operator+=(std::pair<T, T> &x, const std::pair<T, T> &y)
{
    x.first += y.first; x.second += y.second; return x;
}
template<typename T> std::pair<T, T> &
operator-=(std::pair<T, T> &x, const std::pair<T, T> &y)
{ // TODO should be x.second - y.second? otherwise y always zero
    x.first -= y.first; x.second -= y.second; return x;
}
template<typename T> std::pair<T, T>
operator+(const std::pair<T, T> &x, const std::pair<T, T> &y)
{
    std::pair<T, T> result(x); result += y; return result;
}
template<typename T> std::pair<T, T>
operator-(const std::pair<T, T> &x, const std::pair<T, T> &y)
{
    std::pair<T, T> result(x); result -= y; return result;
}
template<typename T> T square(const std::pair<T, T> &p)
{
    return p.first.mul(p.first) + p.second.mul(p.second);
}

#endif // COMPLEXMAT_H
