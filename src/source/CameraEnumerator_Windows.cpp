#include "source/CameraEnumerator.hpp"

// Compiled only on Windows (selected in CMakeLists.txt), so no _WIN32 guard is needed.

#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>

#include <opencv2/videoio.hpp>

namespace livim {
namespace {

std::string wideToUtf8(const wchar_t* w, UINT32 len) {
    if (!w || len == 0) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, static_cast<int>(len), s.data(), n, nullptr, nullptr);
    return s;
}

} // namespace

std::vector<CameraDevice> enumerateCameras() {
    std::vector<CameraDevice> out;

    // Qt's GUI thread already initialized COM (STA), so CoInitializeEx usually returns S_FALSE --
    // still a success that must be balanced with CoUninitialize; RPC_E_CHANGED_MODE is a failure.
    const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool balanceCo = SUCCEEDED(hrCo);

    const bool mfStarted = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));

    IMFAttributes* attr = nullptr;
    if (mfStarted && SUCCEEDED(MFCreateAttributes(&attr, 1))) {
        attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

        IMFActivate** devices = nullptr;
        UINT32 count = 0;
        if (SUCCEEDED(MFEnumDeviceSources(attr, &devices, &count))) {
            for (UINT32 i = 0; i < count; ++i) {
                std::string name;
                wchar_t* friendly = nullptr;
                UINT32 cch = 0;
                if (SUCCEEDED(devices[i]->GetAllocatedString(
                        MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendly, &cch))) {
                    name = wideToUtf8(friendly, cch);
                    CoTaskMemFree(friendly);
                }
                if (name.empty()) name = "Camera " + std::to_string(i);
                out.push_back(CameraDevice{static_cast<int>(i), std::move(name)});

                if (devices[i]) devices[i]->Release();
            }
            CoTaskMemFree(devices);
        }
        attr->Release();
    }

    if (mfStarted) MFShutdown();
    if (balanceCo) CoUninitialize();
    return out;
}

std::vector<int> preferredCaptureApis() {
    return {cv::CAP_MSMF, cv::CAP_DSHOW};
}

} // namespace livim
