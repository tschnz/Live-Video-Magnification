# Overlay triplet: Intel macOS, dynamic linkage; release-only deps when LIVIM_CI_RELEASE_ONLY is set.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

if(DEFINED ENV{LIVIM_CI_RELEASE_ONLY})
    set(VCPKG_BUILD_TYPE release)
endif()
