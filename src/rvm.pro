QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rvm
TEMPLATE = app
DEFINES += APP_VERSION=\\\"1.0\\\"

INCLUDEPATH += \
    $$PWD/main \
    $$PWD/main/helper \
    $$PWD/main/magnification \
    $$PWD/main/other \
    $$PWD/main/threads \
    $$PWD/main/ui \
    $$PWD/external \
    $$PWD/external/qxtSlider

SOURCES += \
    main/main.cpp \
    main/helper/MatToQImage.cpp \
    main/helper/SharedImageBuffer.cpp \
    main/magnification/Magnificator.cpp \
    main/magnification/RieszPyramid.cpp \
    main/magnification/SpatialFilter.cpp \
    main/magnification/TemporalFilter.cpp \
    main/threads/CaptureThread.cpp \
    main/threads/PlayerThread.cpp \
    main/threads/ProcessingThread.cpp \
    main/threads/SavingThread.cpp \
    main/ui/CameraConnectDialog.cpp \
    main/ui/CameraView.cpp \
    main/ui/FrameLabel.cpp \
    main/ui/MagnifyOptions.cpp \
    main/ui/MainWindow.cpp \
    main/ui/VideoView.cpp \
    external/qxtSlider/qxtglobal.cpp \
    external/qxtSlider/qxtspanslider.cpp

HEADERS += \
    main/helper/ComplexMat.h \
    main/helper/MatToQImage.h \
    main/helper/SharedImageBuffer.h \
    main/magnification/Magnificator.h \
    main/magnification/RieszPyramid.h \
    main/magnification/SpatialFilter.h \
    main/magnification/TemporalFilter.h \
    main/threads/CaptureThread.h \
    main/threads/PlayerThread.h \
    main/threads/ProcessingThread.h \
    main/threads/SavingThread.h \
    main/ui/CameraConnectDialog.h \
    main/ui/CameraView.h \
    main/ui/FrameLabel.h \
    main/ui/MagnifyOptions.h \
    main/ui/MainWindow.h \
    main/ui/VideoView.h \
    main/other/Buffer.h \
    main/other/Config.h \
    main/other/Structures.h \
    external/qxtSlider/qxtglobal.h \
    external/qxtSlider/qxtnamespace.h \
    external/qxtSlider/qxtspanslider.h \
    external/qxtSlider/qxtspanslider_p.h

FORMS += \
    main/ui/MainWindow.ui \
    main/ui/CameraView.ui \
    main/ui/CameraConnectDialog.ui \
    main/ui/MagnifyOptions.ui \
    main/ui/VideoView.ui

# Spare me those nasty C++ compiler warnings and pray instead
#QMAKE_CXXFLAGS += -W2

# OpenCV Configuration using pkg-config
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}
