#include "MatToQImage.h"
// Qt
#include <QDebug>

QImage MatToQImage(const cv::Mat& mat) {

    QImage::Format format = QImage::Format_Invalid;
    switch (mat.type()) {
        case CV_8UC4:
            format = QImage::Format_RGBA8888;
            break;
        case CV_8UC3: 
            format = QImage::Format_BGR888;
            break;
        case CV_8UC1:
            format = QImage::Format_Grayscale8;
            break;
        case CV_32FC1:
        {
            // Normalize the float values to the range [0, 255]
            cv::Mat normalizedMat;
            cv::normalize(mat, normalizedMat, 0, 255, cv::NORM_MINMAX);
            normalizedMat.convertTo(normalizedMat, CV_8UC1);
            return QImage(normalizedMat.data, normalizedMat.cols, normalizedMat.rows,
                          static_cast<int>(normalizedMat.step),
                          QImage::Format_Grayscale8).copy();
            break;
        }
        default:
            qWarning() << "MatToQImage: Unsupported Mat type:" << mat.type();
            return QImage();
        }

    return QImage(mat.data, mat.cols, mat.rows,
                static_cast<int>(mat.step),
                format).copy();
}

