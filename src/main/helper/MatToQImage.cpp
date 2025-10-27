#include "MatToQImage.h"
// Qt
#include <QDebug>

QImage MatToQImage(const cv::Mat& mat) {
    switch (mat.type()) {
        // 8-bit, 4 channel
        case CV_8UC4: {
            QImage img(mat.data, mat.cols, mat.rows, 
                      static_cast<int>(mat.step), 
                      QImage::Format_ARGB32);
            return img.copy();
        }
        
        // 8-bit, 3 channel
        case CV_8UC3: {
            QImage img(mat.data, mat.cols, mat.rows, 
                      static_cast<int>(mat.step), 
                      QImage::Format_RGB888);
            return img.rgbSwapped(); // BGR to RGB
        }
        
        // 8-bit, 1 channel
        case CV_8UC1: {
            QImage img(mat.data, mat.cols, mat.rows, 
                      static_cast<int>(mat.step), 
                      QImage::Format_Grayscale8);
            return img.copy();
        }
        
        default:
            qWarning("matToQImage() - cv::Mat type not handled: %d", mat.type());
            break;
    }
    
    return QImage();
}

