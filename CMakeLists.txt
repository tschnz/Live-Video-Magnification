cmake_minimum_required(VERSION 3.16)

project(LiveVideoMagnification VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 Setup
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt6_standard_project_setup()
set(CMAKE_AUTOUIC OFF)

# OpenCV Setup
set(OpenCV_DIR "C:/Workspace/Programs/opencv/build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

# Define the executable
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
qt6_add_ui(${PROJECT_NAME} SOURCES ${UI_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${OpenCV_LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION /Applications
)

# Copy Qt and OpenCV libraries to the executable folder
# if(WIN32)
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:Qt6::Widgets>
#         $<TARGET_FILE_DIR:${PROJECT_NAME}>
#     )
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:Qt6::Gui>
#         $<TARGET_FILE_DIR:${PROJECT_NAME}>
#     )
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:Qt6::Core>
#         $<TARGET_FILE_DIR:${PROJECT_NAME}>
#     )

#     foreach(opencv_lib ${OpenCV_LIBS})
#         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:${opencv_lib}>
#             $<TARGET_FILE_DIR:${PROJECT_NAME}>
#         )
#     endforeach()
# endif()