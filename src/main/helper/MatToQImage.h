#pragma once

// Qt
#include <QtGui/QImage>
// OpenCV
#include <opencv2/opencv.hpp>

using namespace cv;

QImage MatToQImage(const Mat &);