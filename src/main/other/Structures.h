#pragma once

// Qt
#include <QtCore/QRect>

struct ImageProcessingSettings {
  double amplification;
  double coWavelength;
  double coLow;
  double coHigh;
  double chromAttenuation;
  int frameWidth;
  int frameHeight;
  double framerate;
  int levels;

  ImageProcessingSettings()
      : amplification(0.0), coWavelength(0.0), coLow(0.1), coHigh(0.4),
        chromAttenuation(0.0), frameWidth(0), frameHeight(0), framerate(0.0),
        levels(4) {}
};

struct ImageProcessingFlags {
  bool grayscaleOn;
  bool colorMagnifyOn;
  bool laplaceMagnifyOn;
  bool rieszMagnifyOn;

  ImageProcessingFlags()
      : grayscaleOn(false), colorMagnifyOn(false), laplaceMagnifyOn(false),
        rieszMagnifyOn(false) {}
};

struct MouseData {
  QRect selectionBox;
  bool leftButtonRelease;
  bool rightButtonRelease;
};

struct ThreadStatisticsData {
  int averageFPS;
  double nFramesProcessed;
  double averageVidProcessingFPS;
};