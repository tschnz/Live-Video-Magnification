#pragma once

#include <opencv2/core.hpp>

// Complex/quaternionic matrix modelled as a pair of cv::Mats (port of the reference ComplexMat
// helper). The accessors and element-wise operators let the Riesz code read like complex arithmetic.
namespace livim {

typedef std::pair<cv::Mat, cv::Mat> ComplexMat; // (real, imaginary)
typedef std::pair<cv::Mat, cv::Mat> CompExpMat; // (cosine, sine)

template <typename T> T real(const std::pair<T, T>& p) { return p.first; }
template <typename T> T& real(std::pair<T, T>& p) { return p.first; }
template <typename T> T imag(const std::pair<T, T>& p) { return p.second; }
template <typename T> T& imag(std::pair<T, T>& p) { return p.second; }
template <typename T> T cos(const std::pair<T, T>& p) { return p.first; }
template <typename T> T& cos(std::pair<T, T>& p) { return p.first; }
template <typename T> T sin(const std::pair<T, T>& p) { return p.second; }
template <typename T> T& sin(std::pair<T, T>& p) { return p.second; }
template <typename T> T vert(const std::pair<T, T>& p) { return p.first; }
template <typename T> T& vert(std::pair<T, T>& p) { return p.first; }
template <typename T> T hori(const std::pair<T, T>& p) { return p.second; }
template <typename T> T& hori(std::pair<T, T>& p) { return p.second; }

template <typename T> std::pair<T, T> clone(const std::pair<T, T>& rhs) {
    return {rhs.first.clone(), rhs.second.clone()};
}

template <typename T>
std::pair<T, T>& operator+=(std::pair<T, T>& x, const std::pair<T, T>& y) {
    cv::add(x.first, y.first, x.first);
    cv::add(x.second, y.second, x.second);
    return x;
}
template <typename T>
std::pair<T, T>& operator-=(std::pair<T, T>& x, const std::pair<T, T>& y) {
    cv::subtract(x.first, y.first, x.first);
    cv::subtract(x.second, y.second, x.second);
    return x;
}
template <typename T>
std::pair<T, T>& operator*=(std::pair<T, T>& x, const std::pair<T, T>& y) {
    cv::multiply(x.first, y.first, x.first);
    cv::multiply(x.second, y.second, x.second);
    return x;
}
template <typename T, typename T2>
std::pair<T, T>& operator*=(std::pair<T, T>& x, const T2& y) {
    cv::multiply(x.first, y, x.first);
    cv::multiply(x.second, y, x.second);
    return x;
}
template <typename T>
std::pair<T, T>& operator/=(std::pair<T, T>& x, const std::pair<T, T>& y) {
    cv::divide(x.first, y.first, x.first);
    cv::divide(x.second, y.second, x.second);
    return x;
}
template <typename T, typename T2>
std::pair<T, T>& operator/=(std::pair<T, T>& x, const T2& y) {
    cv::divide(x.first, y, x.first);
    cv::divide(x.second, y, x.second);
    return x;
}

template <typename T>
std::pair<T, T> operator+(const std::pair<T, T>& x, const std::pair<T, T>& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()}; // clone so result does not alias x
    result += y;
    return result;
}
template <typename T>
std::pair<T, T> operator-(const std::pair<T, T>& x, const std::pair<T, T>& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()};
    result -= y;
    return result;
}
template <typename T>
std::pair<T, T> operator*(const std::pair<T, T>& x, const std::pair<T, T>& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()};
    result *= y;
    return result;
}
template <typename T, typename T2>
std::pair<T, T> operator*(const std::pair<T, T>& x, const T2& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()};
    result *= y;
    return result;
}
template <typename T>
std::pair<T, T> operator/(const std::pair<T, T>& x, const std::pair<T, T>& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()};
    result /= y;
    return result;
}
template <typename T, typename T2>
std::pair<T, T> operator/(const std::pair<T, T>& x, const T2& y) {
    std::pair<T, T> result{x.first.clone(), x.second.clone()};
    result /= y;
    return result;
}

// |p|^2 element-wise: first^2 + second^2.
template <typename T> T square(const std::pair<T, T>& p) {
    T a, b;
    cv::multiply(p.first, p.first, a);
    cv::multiply(p.second, p.second, b);
    cv::add(a, b, a);
    return a;
}

} // namespace livim
