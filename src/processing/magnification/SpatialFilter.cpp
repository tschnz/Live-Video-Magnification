#include "processing/magnification/SpatialFilter.hpp"

namespace livim {

int calculateMaxLevels(cv::Size s) {
    if (s.width > 5 && s.height > 5) {
        const cv::Size halved((1 + s.width) / 2, (1 + s.height) / 2);
        return 1 + calculateMaxLevels(halved);
    }
    return 0;
}

void buildGaussPyrFromImg(const cv::Mat& img, const int levels, std::vector<cv::Mat>& pyr) {
    pyr.clear();
    cv::Mat currentLevel = img;

    for (int level = 0; level < levels; ++level) {
        cv::Mat down;
        cv::pyrDown(currentLevel, down);
        pyr.push_back(down);
        currentLevel = down;
    }
}

void buildLaplacePyrFromImg(const cv::Mat& img, const int levels, std::vector<cv::Mat>& pyr) {
    pyr.clear();
    cv::Mat currentLevel = img;

    for (int level = 0; level < levels; ++level) {
        cv::Mat down, up;
        cv::pyrDown(currentLevel, down);
        cv::pyrUp(down, up, currentLevel.size());
        cv::Mat laplace = currentLevel - up;
        pyr.push_back(laplace);
        currentLevel = down;
    }
    pyr.push_back(currentLevel);
}

void buildImgFromGaussPyr(const cv::Mat& pyr, const int levels, cv::Mat& dst, cv::Size size) {
    cv::Mat currentLevel = pyr.clone();

    for (int level = 0; level < levels; ++level) {
        cv::Mat up;
        cv::pyrUp(currentLevel, up);
        currentLevel = up;
    }
    cv::resize(currentLevel, currentLevel, size); // absorbs pyrUp rounding drift
    currentLevel.copyTo(dst);
}

void buildImgFromLaplacePyr(const std::vector<cv::Mat>& pyr, const int levels, cv::Mat& dst) {
    cv::Mat currentLevel = pyr[levels];

    for (int level = levels - 1; level >= 0; --level) {
        cv::Mat up;
        cv::pyrUp(currentLevel, up, pyr[level].size());
        currentLevel = up + pyr[level];
    }
    dst = currentLevel.clone();
}

void img2tempMat(const cv::Mat& frame, cv::Mat& dst, int maxImages) {
    cv::Mat reshaped = frame.reshape(frame.channels(), frame.cols * frame.rows).clone();

    if (!(frame.type() == CV_32F || frame.type() == CV_32FC1 || frame.type() == CV_32FC3)) {
        if (frame.channels() == 1)
            reshaped.convertTo(reshaped, CV_32FC1);
        else
            reshaped.convertTo(reshaped, CV_32FC3);
    }

    if (dst.cols == 0) {
        reshaped.copyTo(dst);
    }
    else {
        cv::hconcat(dst, reshaped, dst);
    }

    // Once full, drop the oldest column.
    if (dst.cols > maxImages && maxImages > 0) {
        dst.colRange(1, dst.cols).copyTo(dst);
    }
}

void tempMat2img(const cv::Mat& src, int position, const cv::Size& frameSize, cv::Mat& frame) {
    cv::Mat line = src.col(position).clone();
    frame = line.reshape(line.channels(), frameSize.height).clone();
}

} // namespace livim
