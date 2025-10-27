cmake_minimum_required(VERSION 3.21)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(LiveVideoMagnification VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# STATIC BUILD CONFIGURATION (Cross-platform)
# ============================================================================
option(BUILD_STATIC "Build static executable" ON)

if(BUILD_STATIC)
    # Force static linking for all libraries
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    
    # OpenCV static configuration
    set(OpenCV_STATIC ON CACHE BOOL "Use static OpenCV libraries" FORCE)
    
    message(STATUS "Building STATIC executable")
else()
    message(STATUS "Building DYNAMIC executable")
endif()

# ============================================================================
# Dependencies
# ============================================================================
find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC OFF)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# Gather all files necessary for the project
# ============================================================================
set(SOURCES
    src/main/main.cpp
    src/main/helper/MatToQImage.cpp
    src/main/helper/SharedImageBuffer.cpp
    src/main/magnification/Magnificator.cpp
    src/main/magnification/RieszPyramid.cpp
    src/main/magnification/SpatialFilter.cpp
    src/main/magnification/TemporalFilter.cpp
    src/main/threads/CaptureThread.cpp
    src/main/threads/PlayerThread.cpp
    src/main/threads/ProcessingThread.cpp
    src/main/threads/SavingThread.cpp
    src/main/ui/CameraConnectDialog.cpp
    src/main/ui/CameraView.cpp
    src/main/ui/FrameLabel.cpp
    src/main/ui/MagnifyOptions.cpp
    src/main/ui/MainWindow.cpp
    src/main/ui/VideoView.cpp
    src/external/qxtSlider/qxtglobal.cpp
    src/external/qxtSlider/qxtspanslider.cpp
)
set(HEADERS
    src/main/helper/ComplexMat.h
    src/main/helper/MatToQImage.h
    src/main/helper/SharedImageBuffer.h
    src/main/magnification/Magnificator.h
    src/main/magnification/RieszPyramid.h
    src/main/magnification/SpatialFilter.h
    src/main/magnification/TemporalFilter.h
    src/main/threads/CaptureThread.h
    src/main/threads/PlayerThread.h
    src/main/threads/ProcessingThread.h
    src/main/threads/SavingThread.h
    src/main/ui/CameraConnectDialog.h
    src/main/ui/CameraView.h
    src/main/ui/FrameLabel.h
    src/main/ui/MagnifyOptions.h
    src/main/ui/MainWindow.h
    src/main/ui/VideoView.h
    src/main/other/Buffer.h
    src/main/other/Config.h
    src/main/other/Structures.h
    src/external/qxtSlider/qxtglobal.h
    src/external/qxtSlider/qxtnamespace.h
    src/external/qxtSlider/qxtspanslider.h
    src/external/qxtSlider/qxtspanslider_p.h
)
set(UI_FILES
    src/main/ui/MainWindow.ui
    src/main/ui/CameraView.ui
    src/main/ui/CameraConnectDialog.ui
    src/main/ui/MagnifyOptions.ui
    src/main/ui/VideoView.ui
)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})

qt_add_ui(${PROJECT_NAME} SOURCES ${UI_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/ui
)

# ============================================================================
# Link libraries
# ============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
        opencv_world_d
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Widgets
        opencv_world
    )
endif()


# ============================================================================
# Compiler and linker options
# ============================================================================
if(MSVC)    
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /WX-
        $<$<CONFIG:Release>:/O2 /GL>
        $<$<CONFIG:Release>:/arch:AVX2>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Debug>:/DEBUG>
        /SUBSYSTEM:WINDOWS
        /ENTRY:mainCRTStartup
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-flto>
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# ============================================================================
# Installation
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# For dynamic builds, use Qt's deployment script
if(NOT BUILD_STATIC)
    qt_generate_deploy_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})

    # Install OpenCV DLLs using imported target file locations
    install(FILES
        $<TARGET_FILE:opencv_world>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel
    )
endif()