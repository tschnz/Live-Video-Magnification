cmake_minimum_required(VERSION 3.21)

project(LiveVideoMagnification VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV Setup
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    if(NOT OpenCV_DIR)
        message(FATAL_ERROR "OpenCV not found and OpenCV_DIR is not set. Please specify the path to the OpenCV build directory.")
    else()
        find_package(OpenCV REQUIRED PATHS ${OpenCV_DIR} NO_DEFAULT_PATH)
        if(NOT OpenCV_FOUND)
            message(FATAL_ERROR "OpenCV not found in the specified OpenCV_DIR: ${OpenCV_DIR}")
        endif()
    endif()
endif()

# Qt6 Setup
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()
set(CMAKE_AUTOUIC OFF)

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

# Gather all files necessary for the project
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

target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/main/ui)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${OpenCV_LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)