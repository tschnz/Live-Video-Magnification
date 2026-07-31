#pragma once

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Spatial-pyramid helpers for Eulerian video magnification (reference implementation,
// src/main/magnification/SpatialFilter.cpp). CV_32F images.
namespace livim {

// Maximum number of pyramid levels a frame of the given size supports (halve while both
// dimensions stay above 5).
int calculateMaxLevels(cv::Size size);

// Gaussian pyramid: `levels` successively pyrDown'd images (index levels-1 is the smallest).
// The original full-resolution image is NOT stored.
void buildGaussPyrFromImg(const cv::Mat& img, int levels, std::vector<cv::Mat>& pyr);

// Reconstruct a full-resolution image from the smallest Gaussian level by pyrUp'ing `levels`
// times, then resizing to `size` to absorb rounding drift.
void buildImgFromGaussPyr(const cv::Mat& pyr, int levels, cv::Mat& dst, cv::Size size);

// Laplacian pyramid: `levels` detail bands (index 0 = finest) plus the coarsest residual appended
// at index `levels`, so the vector holds levels+1 Mats.
void buildLaplacePyrFromImg(const cv::Mat& img, int levels, std::vector<cv::Mat>& pyr);

// Collapse a Laplacian pyramid (levels+1 Mats) back to a full-resolution image.
void buildImgFromLaplacePyr(const std::vector<cv::Mat>& pyr, int levels, cv::Mat& dst);

// Reshape one frame into a single column (rows*cols x 1, same channels) and hconcat it onto `dst`,
// keeping at most `maxImages` columns -- the rolling temporal window for the colour mode.
void img2tempMat(const cv::Mat& frame, cv::Mat& dst, int maxImages);

// Inverse of img2tempMat for one column: reshape column `position` back to an image.
void tempMat2img(const cv::Mat& src, int position, const cv::Size& frameSize, cv::Mat& frame);

} // namespace livim
