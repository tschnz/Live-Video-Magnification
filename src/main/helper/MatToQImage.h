#pragma once

// Qt
#include <QtGui/QImage>
// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

QImage MatToQImage(const Mat &);