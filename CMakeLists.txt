cmake_minimum_required(VERSION 3.25)

# Must be set before project(); >= Qt's minimum (Qt 6.11 -> macOS 12).
if (NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment version")
endif()

project(LiViM VERSION 0.1.0 LANGUAGES CXX)

if (APPLE)
    enable_language(OBJCXX)   # camera backend is Objective-C++
endif()

if (WIN32)
    enable_language(RC)       # .exe icon resource
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIVIM_USE_SYSTEM_DEPS
    "Build against system Qt6/OpenCV/FFmpeg instead of vcpkg (explicit local opt-in)" OFF)

find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc videoio)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
if (UNIX AND NOT APPLE)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
endif()

qt_standard_project_setup()

# ---------------------------------------------------------------------------
# Dependency provenance: Qt6/OpenCV/FFmpeg come from vcpkg (the vcpkg toolchain
# file was injected) or from the system distro (no vcpkg toolchain). This drives
# the packaging/install logic below so a system-deps build still yields a
# self-contained /opt bundle. System dependencies are explicit local opt-in;
# release/CI builds are required to use the vcpkg presets.
# ---------------------------------------------------------------------------
if (LIVIM_USE_SYSTEM_DEPS)
    set(LIVIM_DEPS_SOURCE "system")
elseif (DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(LIVIM_DEPS_SOURCE "vcpkg")
else()
    message(FATAL_ERROR
        "No vcpkg toolchain is active. Use the explicit system-deps preset with "
        "-DLIVIM_USE_SYSTEM_DEPS=ON, or configure with a vcpkg preset.")
endif()

if (DEFINED ENV{GITHUB_ACTIONS} AND LIVIM_DEPS_SOURCE STREQUAL "system")
    message(FATAL_ERROR
        "GitHub Actions builds must use vcpkg dependencies. Use the vcpkg release preset.")
endif()

if (LIVIM_DEPS_SOURCE STREQUAL "system" AND UNIX AND NOT APPLE
        AND Qt6_VERSION VERSION_LESS "6.5")
    message(FATAL_ERROR
        "System-deps Linux builds require Qt6 >= 6.5; found Qt ${Qt6_VERSION}.")
endif()

message(STATUS "LiViM: dependency source = ${LIVIM_DEPS_SOURCE}")

# Strict warnings for our own code only; dependency headers arrive via -isystem.
add_library(livim_warnings INTERFACE)
if (MSVC)
    target_compile_options(livim_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(livim_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion)
endif()
option(LIVIM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if (LIVIM_WARNINGS_AS_ERRORS)
    target_compile_options(livim_warnings INTERFACE $<IF:$<BOOL:${MSVC}>,/WX,-Werror>)
endif()

qt_add_executable(livim WIN32 MACOSX_BUNDLE
    src/app/main.cpp
    src/core/Clock.hpp
    src/core/Frame.hpp
    src/core/BoundedQueue.hpp
    src/core/LatestFrameMailbox.hpp
    src/core/AtomicConfig.hpp
    src/core/AtomicSharedPtr.hpp
    src/core/PipelineTypes.hpp
    src/core/FramePool.hpp
    src/core/FramePool.cpp
    src/core/Instrumentation.hpp
    src/core/Instrumentation.cpp
    src/core/IFrameSink.hpp
    src/core/IVideoRenderer.hpp
    src/source/ISource.hpp
    src/source/SourceBase.hpp
    src/source/SourceBase.cpp
    src/source/FileSource.hpp
    src/source/FileSource.cpp
    src/source/CameraSource.hpp
    src/source/CameraSource.cpp
    src/source/CameraEnumerator.hpp
    src/processing/IProcessor.hpp
    src/processing/PreprocessProcessor.hpp
    src/processing/PreprocessProcessor.cpp
    src/processing/GrayscaleProcessor.hpp
    src/processing/GrayscaleProcessor.cpp
    src/processing/MagnificationProcessor.hpp
    src/processing/MagnificationProcessor.cpp
    src/processing/MagnificationParamsUi.hpp
    src/processing/magnification/ComplexMat.hpp
    src/processing/magnification/MagnifyCore.hpp
    src/processing/magnification/SpatialFilter.hpp
    src/processing/magnification/SpatialFilter.cpp
    src/processing/magnification/TemporalFilter.hpp
    src/processing/magnification/TemporalFilter.cpp
    src/processing/magnification/RieszPyramid.hpp
    src/processing/magnification/RieszPyramid.cpp
    src/processing/ProcessingChain.hpp
    src/processing/ProcessingChain.cpp
    src/processing/ChainBuilder.hpp
    src/processing/ChainBuilder.cpp
    src/export/ExportTypes.hpp
    src/export/IExportFrameSource.hpp
    src/export/FileExportFrameSource.hpp
    src/export/FileExportFrameSource.cpp
    src/export/BufferExportFrameSource.hpp
    src/export/BufferExportFrameSource.cpp
    src/export/RecordingBuffer.hpp
    src/export/RecordingBuffer.cpp
    src/export/Exporter.hpp
    src/export/Exporter.cpp
    src/pipeline/PlaybackController.hpp
    src/pipeline/PlaybackController.cpp
    src/ui/Theme.hpp
    src/ui/Theme.cpp
    src/ui/Icons.hpp
    src/ui/Icons.cpp
    src/ui/DisplayWidget.hpp
    src/ui/DisplayWidget.cpp
    src/ui/CameraSelectDialog.hpp
    src/ui/CameraSelectDialog.cpp
    src/ui/SourceControlsView.hpp
    src/ui/SourceControlsView.cpp
    src/ui/FileControlsView.hpp
    src/ui/FileControlsView.cpp
    src/ui/CameraControlsView.hpp
    src/ui/CameraControlsView.cpp
    src/ui/TimelineView.hpp
    src/ui/TimelineView.cpp
    src/ui/ToggleSwitch.hpp
    src/ui/ToggleSwitch.cpp
    src/ui/SegmentedControl.hpp
    src/ui/SegmentedControl.cpp
    src/ui/SliderRow.hpp
    src/ui/SliderRow.cpp
    src/ui/RangeSlider.hpp
    src/ui/RangeSlider.cpp
    src/ui/MagnificationControls.hpp
    src/ui/MagnificationControls.cpp
    src/ui/ProcessingPanel.hpp
    src/ui/ProcessingPanel.cpp
    src/ui/StatusHealth.hpp
    src/ui/StatusStrip.hpp
    src/ui/StatusStrip.cpp
    src/ui/ExportSettingsDialog.hpp
    src/ui/ExportSettingsDialog.cpp
    src/ui/ExportProgressDialog.hpp
    src/ui/ExportProgressDialog.cpp
    src/ui/MainWindow.hpp
    src/ui/MainWindow.cpp
)

target_include_directories(livim PRIVATE src)

# Camera backend: exactly one platform implementation.
if (WIN32)
    target_sources(livim PRIVATE src/source/CameraEnumerator_Windows.cpp
                                 "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/livim.rc")
elseif (APPLE)
    target_sources(livim PRIVATE src/source/CameraEnumerator_macOS.mm)
else ()
    target_sources(livim PRIVATE src/source/CameraEnumerator_Linux.cpp)
endif ()

target_link_libraries(livim PRIVATE
    livim_warnings
    opencv_core opencv_imgproc opencv_videoio
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
)

if (WIN32)
    target_link_libraries(livim PRIVATE mf mfplat mfuuid ole32)   # Media Foundation + COM
elseif (APPLE)
    target_link_libraries(livim PRIVATE
        "-framework AVFoundation" "-framework Foundation"
        "-framework CoreMedia"    "-framework CoreVideo")
endif()

# macOS bundle: Info.plist carries NSCameraUsageDescription (required for camera access).
if (APPLE)
    set(_livim_icns "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/livim.icns")
    target_sources(livim PRIVATE "${_livim_icns}")
    set_source_files_properties("${_livim_icns}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(livim PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST          "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME         "LiViM"
        MACOSX_BUNDLE_GUI_IDENTIFIER      "io.github.tschnz.livim"
        MACOSX_BUNDLE_ICON_FILE           "livim.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION      "${PROJECT_VERSION}")
endif()

# Copy the Qt platform plugins next to the binary so the app runs from the build tree (vcpkg does
# not deploy Qt plugins). Packaging bundles the full runtime closure separately below.
function(livim_deploy_qt_plugin plugin_target subdir)
    if (TARGET ${plugin_target})
        add_custom_command(TARGET livim POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_livim_plugin_root}/${subdir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${plugin_target}>"
                    "${_livim_plugin_root}/${subdir}/"
            VERBATIM)
    endif()
endfunction()

if (APPLE)
    set(_livim_plugin_root "$<TARGET_BUNDLE_CONTENT_DIR:livim>/PlugIns")
else()
    set(_livim_plugin_root "$<TARGET_FILE_DIR:livim>")
endif()

if (WIN32)
    livim_deploy_qt_plugin(Qt6::QWindowsIntegrationPlugin platforms)
elseif (APPLE)
    livim_deploy_qt_plugin(Qt6::QCocoaIntegrationPlugin platforms)
elseif (UNIX)
    livim_deploy_qt_plugin(Qt6::QXcbIntegrationPlugin    platforms)
    livim_deploy_qt_plugin(Qt6::QXcbGlxIntegrationPlugin xcbglintegrations)
    livim_deploy_qt_plugin(Qt6::QXcbEglIntegrationPlugin xcbglintegrations)
    livim_deploy_qt_plugin(Qt6::QWaylandIntegrationPlugin         platforms)
    livim_deploy_qt_plugin(Qt6::QWaylandXdgShellIntegrationPlugin wayland-shell-integration)
    livim_deploy_qt_plugin(Qt6::QWaylandEglClientBufferPlugin     wayland-graphics-integration-client)
    livim_deploy_qt_plugin(Qt6::QWaylandBradientDecorationPlugin  wayland-decoration-client)
    livim_deploy_qt_plugin(Qt6::QXdgDesktopPortalThemePlugin      platformthemes)
endif()

# --- Install & packaging (CPack) -----------------------------------------------------------------
include(GNUInstallDirs)

# Bake the final RPATH at build time so the install step performs no RPATH rewrite.
# vcpkg: keeps the vcpkg lib dir (build-tree runs) plus $ORIGIN/../lib (installed layout); Debug links vcpkg's debug libs.
# system: just $ORIGIN/../lib so the /opt bundle is self-contained.
if (UNIX AND NOT APPLE)
    if (LIVIM_DEPS_SOURCE STREQUAL "vcpkg")
        set_target_properties(livim PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$<$<CONFIG:Debug>:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib;>${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib;$ORIGIN/../lib")
    else()
        set_target_properties(livim PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN/../lib")
    endif()
endif()

if (WIN32)
    install(TARGETS livim RUNTIME DESTINATION .)   # flat layout: exe + DLLs at the package root
else()
    install(TARGETS livim
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# macOS (macdeployqt) and Linux (Qt deploy runtime) bundle the full runtime closure. Windows'
# windeployqt is Qt-only and would miss the OpenCV/ffmpeg/vcpkg DLLs, so Windows deploys manually below.
# macOS always uses Qt's deploy helper; so does the vcpkg path. The Linux *system* path skips it:
# Qt's helper aborts on distro Qt6 plugins that ship without a RUNPATH (e.g. libQt6Xdg*.so, common on
# Arch/Manjaro), so the manual install(CODE) block below owns the full closure there instead.
if (NOT WIN32)
    if (APPLE OR LIVIM_DEPS_SOURCE STREQUAL "vcpkg")
        qt_generate_deploy_app_script(
            TARGET livim
            OUTPUT_SCRIPT _livim_deploy_script
            NO_TRANSLATIONS
            NO_UNSUPPORTED_PLATFORM_ERROR)
        install(SCRIPT ${_livim_deploy_script})
    endif()
endif()

# Qt's Linux deploy bundles only the default xcb plugin; inject the native-Wayland set + the portal
# file-dialog theme, bundle their extra Qt libs, and rewrite RPATHs so the tree stays relocatable.
if (UNIX AND NOT APPLE)
    find_program(PATCHELF_EXECUTABLE patchelf REQUIRED)
    if (LIVIM_DEPS_SOURCE STREQUAL "vcpkg")
        set(_livim_qt_plugin_src "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
        set(_livim_dep_dirs      "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
        set(_livim_dep_excludes  "^/(usr|lib|lib64)/" "ld-linux")
        # qt_generate_deploy_app_script (above) already bundles the default xcb plugin + the exe
        # closure for the vcpkg path, so only the native-Wayland set is added here.
        set(_livim_plugin_wl
            "platforms/libqwayland.so"
            "wayland-shell-integration/libxdg-shell.so"
            "wayland-graphics-integration-client/libqt-plugin-wayland-egl.so"
            "wayland-decoration-client/libbradient.so"
            "platformthemes/libqxdgdesktopportal.so")
        set(_livim_exe_walk "")   # qt_generate owns the executable closure on this path
    else()
        # System Qt: derive the plugins dir from an imported plugin target (distro-agnostic),
        # falling back to qmake's QT_INSTALL_PLUGINS. Resolve the runtime closure from the
        # platform default library search paths (no vcpkg lib dir to hint).
        if (TARGET Qt6::QXcbIntegrationPlugin)
            get_target_property(_livim_plugin_loc Qt6::QXcbIntegrationPlugin IMPORTED_LOCATION)
            get_filename_component(_d "${_livim_plugin_loc}" DIRECTORY)      # .../platforms
            get_filename_component(_livim_qt_plugin_src "${_d}" DIRECTORY)   # .../plugins
        else()
            execute_process(COMMAND qmake6 -query QT_INSTALL_PLUGINS
                OUTPUT_VARIABLE _livim_qt_plugin_src OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
        set(_livim_dep_dirs     "")      # use platform default search paths
        set(_livim_dep_excludes "ld-linux")
        # Without qt_generate, the manual block owns the full closure: bundle the default xcb
        # plugin (Qt's helper would have taken it) together with the Wayland set, and walk the
        # executable itself so Qt6/OpenCV/FFmpeg get copied + rpathed into /opt.
        set(_livim_plugin_wl
            "platforms/libqxcb.so"
            "platforms/libqwayland.so"
            "wayland-shell-integration/libxdg-shell.so"
            "wayland-graphics-integration-client/libqt-plugin-wayland-egl.so"
            "wayland-decoration-client/libbradient.so"
            "platformthemes/libqxdgdesktopportal.so")
        set(_livim_exe_walk "EXECUTABLES \"\${_real}/bin/livim\"")
    endif()
    install(CODE "
        set(_pfx  \"\${CMAKE_INSTALL_PREFIX}\")
        set(_real \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
        set(_wl ${_livim_plugin_wl})
        set(_plugfiles)
        foreach(p IN LISTS _wl)
            get_filename_component(_sub \"\${p}\" DIRECTORY)
            get_filename_component(_fn  \"\${p}\" NAME)
            file(INSTALL \"${_livim_qt_plugin_src}/\${p}\" DESTINATION \"\${_pfx}/Qt6/plugins/\${_sub}\")
            execute_process(COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \"\\\$ORIGIN/../../../lib\"
                            \"\${_real}/Qt6/plugins/\${_sub}/\${_fn}\")
            list(APPEND _plugfiles \"\${_real}/Qt6/plugins/\${_sub}/\${_fn}\")
        endforeach()
        file(GET_RUNTIME_DEPENDENCIES
            ${_livim_exe_walk}
            LIBRARIES \${_plugfiles}
            RESOLVED_DEPENDENCIES_VAR _res UNRESOLVED_DEPENDENCIES_VAR _unres
            DIRECTORIES ${_livim_dep_dirs}
            POST_EXCLUDE_REGEXES ${_livim_dep_excludes})
        foreach(_dep IN LISTS _res)
            get_filename_component(_dn \"\${_dep}\" NAME)
            if(NOT EXISTS \"\${_real}/lib/\${_dn}\")
                file(INSTALL \"\${_dep}\" DESTINATION \"\${_pfx}/lib\" FOLLOW_SYMLINK_CHAIN)
                execute_process(COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \"\\\$ORIGIN\" \"\${_real}/lib/\${_dn}\")
            endif()
        endforeach()
        execute_process(COMMAND \"${PATCHELF_EXECUTABLE}\" --set-rpath \"\\\$ORIGIN/../lib\" \"\${_real}/bin/livim\")
    ")

    # Desktop entry + icons live inside the /opt bundle; the maintainer scripts symlink them into /usr.
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/livim.desktop"
            DESTINATION share/applications)
    foreach(_sz 16 32 48 64 128 256 512)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/icons/livim-${_sz}.png"
                DESTINATION "share/icons/hicolor/${_sz}x${_sz}/apps" RENAME livim.png)
    endforeach()
endif()

# Windows manual deploy: resolve the exe's full DLL closure from the vcpkg bin dir and copy it next
# to the exe, plus the platform plugin and an app-local MSVC runtime.
if (WIN32)
    set(_livim_qt_plugin_src "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
    set(_livim_vcpkg_bin     "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf.win" "[Paths]\nPlugins = .\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf.win" DESTINATION . RENAME qt.conf)
    install(CODE "
        if(POLICY CMP0207)
            cmake_policy(SET CMP0207 NEW)
        endif()
        set(_pfx \"\${CMAKE_INSTALL_PREFIX}\")
        file(INSTALL \"${_livim_qt_plugin_src}/platforms/qwindows.dll\" DESTINATION \"\${_pfx}/platforms\")
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"\${_pfx}/livim.exe\"
            MODULES     \"\${_pfx}/platforms/qwindows.dll\"
            RESOLVED_DEPENDENCIES_VAR   _res
            UNRESOLVED_DEPENDENCIES_VAR _unres
            DIRECTORIES \"${_livim_vcpkg_bin}\"
            PRE_EXCLUDE_REGEXES  \"api-ms-\" \"ext-ms-\"
            POST_EXCLUDE_REGEXES \"[Ss]ystem32\")
        foreach(_dep IN LISTS _res)
            file(INSTALL \"\${_dep}\" DESTINATION \"\${_pfx}\")
        endforeach()
        list(LENGTH _res _nres)
        message(STATUS \"livim: bundled \${_nres} runtime DLL(s) next to livim.exe\")
        if(_unres)
            message(STATUS \"livim: unresolved (expected to be system DLLs): \${_unres}\")
        endif()
    ")
endif()

set(CPACK_PACKAGE_NAME                "LiViM")
set(CPACK_PACKAGE_VENDOR              "tschnz")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Live video motion and color magnification")
set(CPACK_PACKAGE_VERSION             "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY   "LiViM")
set(CPACK_PACKAGE_EXECUTABLES         livim "LiViM")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_DIRECTORY           "${CMAKE_SOURCE_DIR}/dist")
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# Uniform artifact naming on every OS/format: <Name>-<Version>-<OS>-<Arch>.<ext>
if (WIN32)
    set(LIVIM_OS "Windows")
elseif (APPLE)
    set(LIVIM_OS "macOS")
else()
    set(LIVIM_OS "Linux")
endif()
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _livim_arch)
if (_livim_arch MATCHES "^(amd64|x86_64|x64)$")
    set(LIVIM_ARCH "x86_64")
elseif (_livim_arch MATCHES "^(arm64|aarch64)$")
    set(LIVIM_ARCH "arm64")
else()
    set(LIVIM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${LIVIM_OS}-${LIVIM_ARCH}")

if (WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "LiViM")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_ICON    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/livim.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/livim.ico")
elseif (APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    # Self-contained under /opt: the bundled Qt/OpenCV/ffmpeg must not shadow system libraries.
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "tschnz <tschnz@users.noreply.github.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "video")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.deb")
    # The RPM is self-contained; it must not advertise or require distro-specific dependencies.
    set(CPACK_RPM_PACKAGE_LICENSE     "AGPL-3.0-or-later")
    set(CPACK_RPM_PACKAGE_GROUP       "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_AUTOREQPROV OFF)
    set(CPACK_RPM_FILE_NAME           "${CPACK_PACKAGE_FILE_NAME}.rpm")

    # Maintainer scripts symlink the /opt bundle into /usr (menu entry, PATH) and remove the links
    # on uninstall. dpkg requires the exec bit on its control scripts.
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postinst.in" "${CMAKE_CURRENT_BINARY_DIR}/linux/postinst"   @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postrm.in"   "${CMAKE_CURRENT_BINARY_DIR}/linux/postrm"     @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/post.in"     "${CMAKE_CURRENT_BINARY_DIR}/linux/rpm-post"   @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postun.in"   "${CMAKE_CURRENT_BINARY_DIR}/linux/rpm-postun" @ONLY)
    file(CHMOD "${CMAKE_CURRENT_BINARY_DIR}/linux/postinst" "${CMAKE_CURRENT_BINARY_DIR}/linux/postrm"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_BINARY_DIR}/linux/postinst;${CMAKE_CURRENT_BINARY_DIR}/linux/postrm")
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE   "${CMAKE_CURRENT_BINARY_DIR}/linux/rpm-post")
    set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/linux/rpm-postun")
endif()

include(CPack)
