#pragma once

#include <string>
#include <vector>

namespace livim {

// `index` is the ordinal to pass to cv::VideoCapture; it matches the enumeration order OpenCV uses.
struct CameraDevice {
    int index = 0;
    std::string name;
};

// One implementation per OS, selected by CMake: Media Foundation, V4L2, AVFoundation.
std::vector<CameraDevice> enumerateCameras();

// The cv::VideoCapture backend ids to try, in order, when opening an enumerated device.
// Plain ints (not cv::VideoCaptureAPIs) so this header stays free of <opencv2/...>.
std::vector<int> preferredCaptureApis();

} // namespace livim
