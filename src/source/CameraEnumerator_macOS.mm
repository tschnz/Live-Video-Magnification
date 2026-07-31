#include "source/CameraEnumerator.hpp"

// Opening a device triggers the camera-permission prompt, which requires NSCameraUsageDescription
// in the app bundle's Info.plist (see cmake/Info.plist.in).

#import <AVFoundation/AVFoundation.h>

#include <string>

#include <opencv2/videoio.hpp>

namespace livim {

std::vector<CameraDevice> enumerateCameras() {
    std::vector<CameraDevice> out;

    // Must match the ordering OpenCV's AVFoundation backend uses so the position here matches the
    // index cv::VideoCapture opens. +devicesWithMediaType: is deprecated but is that order.
    NSArray<AVCaptureDevice*>* devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    int i = 0;
    for (AVCaptureDevice* device in devices) {
        const char* utf8 = [[device localizedName] UTF8String];
        std::string name = (utf8 && *utf8) ? std::string(utf8) : ("Camera " + std::to_string(i));
        out.push_back(CameraDevice{i, std::move(name)});
        ++i;
    }
    return out;
}

std::vector<int> preferredCaptureApis() {
    // CAP_ANY falls back if the AVFoundation backend wasn't compiled into this OpenCV build.
    return {cv::CAP_AVFOUNDATION, cv::CAP_ANY};
}

} // namespace livim
