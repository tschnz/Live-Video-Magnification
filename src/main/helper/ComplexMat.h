#pragma once

#include <opencv2/core/core.hpp>

// Because C++ forbids std::complex<cv::Mat> for who knows what reason ...
//
// ComplexMat and CompExpMat are just two suggestive names for the same
// type.  ComplexMat has real and imaginary parts.  ComplexExp has cosine
// and sine parts, which also happen to be real and imaginary parts.
//
typedef std::pair<cv::Mat, cv::Mat> ComplexMat; // a real and imaginary matrix
typedef std::pair<cv::Mat, cv::Mat> CompExpMat; // a cos and sin matrix

template <typename T> T real(const std::pair<T, T> &p) { return p.first; }
template <typename T> T &real(std::pair<T, T> &p) { return p.first; }
template <typename T> T imag(const std::pair<T, T> &p) { return p.second; }
template <typename T> T &imag(std::pair<T, T> &p) { return p.second; }
template <typename T> T cos(const std::pair<T, T> &p) { return p.first; }
template <typename T> T &cos(std::pair<T, T> &p) { return p.first; }
template <typename T> T sin(const std::pair<T, T> &p) { return p.second; }
template <typename T> T &sin(std::pair<T, T> &p) { return p.second; }
template <typename T> T vert(const std::pair<T, T> &p) { return p.first; }
template <typename T> T &vert(std::pair<T, T> &p) { return p.first; }
template <typename T> T hori(const std::pair<T, T> &p) { return p.second; }
template <typename T> T &hori(std::pair<T, T> &p) { return p.second; }

template <typename T> std::pair<T, T> clone(const std::pair<T, T> &rhs) {
  std::pair<T, T> result;
  result.first = rhs.first.clone();
  result.second = rhs.second.clone();

  return result;
}

// ----------------

template <typename T>
std::pair<T, T> &operator+=(std::pair<T, T> &x, const std::pair<T, T> &y) {
  x.first += y.first;
  x.second += y.second;
  return x;
}

template <typename T>
std::pair<T, T> &operator-=(std::pair<T, T> &x, const std::pair<T, T> &y) {
  x.first -= y.first;
  x.second -= y.second;
  return x;
}
template <typename T>
std::pair<T, T> &operator*=(std::pair<T, T> &x, const std::pair<T, T> &y) {
  cv::multiply(x.first, y.first, x.first);
  cv::multiply(x.second, y.second, x.second);
  return x;
}

template <typename T, typename T2>
std::pair<T, T> &operator*=(std::pair<T, T> &x, const T2 &y) {
  cv::multiply(x.first, y, x.first);
  cv::multiply(x.second, y, x.second);
  return x;
}
template <typename T>
std::pair<T, T> &operator/=(std::pair<T, T> &x, const std::pair<T, T> &y) {
  cv::divide(x.first, y.first, x.first);
  cv::divide(x.second, y.second, x.second);
  return x;
}

template <typename T, typename T2>
std::pair<T, T> &operator/=(std::pair<T, T> &x, const T2 &y) {
  cv::divide(x.first, y, x.first);
  cv::divide(x.second, y, x.second);
  return x;
}

// --------------

template <typename T>
std::pair<T, T> operator+(const std::pair<T, T> &x, const std::pair<T, T> &y) {
  // Clone matrices, otherwise result holds pointer to x and changes this mat as
  // well
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result += y;
  return result;
}

template <typename T>
std::pair<T, T> operator-(const std::pair<T, T> &x, const std::pair<T, T> &y) {
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result -= y;
  return result;
}

template <typename T>
std::pair<T, T> operator*(const std::pair<T, T> &x, const std::pair<T, T> &y) {
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result *= y;
  return result;
}
template <typename T, typename T2>
std::pair<T, T> operator*(const std::pair<T, T> &x, const T2 &y) {
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result *= y;
  return result;
}

template <typename T>
std::pair<T, T> operator/(const std::pair<T, T> &x, const std::pair<T, T> &y) {
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result /= y;
  return result;
}
template <typename T, typename T2>
std::pair<T, T> operator/(const std::pair<T, T> &x, const T2 &y) {
  std::pair<T, T> result;
  result.first = x.first.clone();
  result.second = x.second.clone();
  result /= y;
  return result;
}

template <typename T> T square(const std::pair<T, T> &p) {
  cv::Mat result = p.first.mul(p.first) + p.second.mul(p.second);
  return result;
}
