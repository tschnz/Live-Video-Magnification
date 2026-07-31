#include "source/CameraEnumerator.hpp"

// Compiled only on Linux / non-Apple Unix (selected in CMakeLists.txt).

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <cstring>
#include <string>

#include <opencv2/videoio.hpp>

namespace livim {

std::vector<CameraDevice> enumerateCameras() {
    std::vector<CameraDevice> out;

    // /dev/video<N>: N is exactly the index OpenCV's V4L2 backend opens for cv::VideoCapture(N,
    // CAP_V4L2), so device ordinal and cv index match by construction.
    for (int i = 0; i < 64; ++i) {
        const std::string path = "/dev/video" + std::to_string(i);
        const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        v4l2_capability cap{};
        if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            // Prefer per-node device_caps; older drivers only set the global capabilities field.
            const unsigned caps =
                (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;

            // Filter out metadata/output-only nodes (UVC cameras register several /dev/video nodes).
            if (caps & V4L2_CAP_VIDEO_CAPTURE) {
                std::string name(reinterpret_cast<const char*>(cap.card),
                                 ::strnlen(reinterpret_cast<const char*>(cap.card), sizeof(cap.card)));
                if (name.empty()) name = "Camera " + std::to_string(i);
                out.push_back(CameraDevice{i, std::move(name)});
            }
        }
        ::close(fd);
    }
    return out;
}

std::vector<int> preferredCaptureApis() {
    // CAP_ANY falls back if the V4L2 backend wasn't compiled into this OpenCV build.
    return {cv::CAP_V4L2, cv::CAP_ANY};
}

} // namespace livim
