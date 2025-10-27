#pragma once

// Qt
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
// OpenCV
#include <opencv2/opencv.hpp>
// Local
#include "../helper/SharedImageBuffer.h"
#include "../other/Config.h"
#include "../other/Structures.h"

using namespace cv;

class ImageBuffer;

class CaptureThread : public QThread {
  Q_OBJECT

public:
  CaptureThread(SharedImageBuffer *sharedImageBuffer, int deviceNumber,
                bool dropFrameIfBufferFull, int width, int height,
                int fpsLimit);
  void stop();
  bool connectToCamera();
  bool disconnectCamera();
  bool isCameraConnected();
  int getInputSourceWidth();
  int getInputSourceHeight();

private:
  void updateFPS(int);
  SharedImageBuffer *sharedImageBuffer;
  VideoCapture cap;
  Mat grabbedFrame;
  QElapsedTimer t;
  QMutex doStopMutex;
  QQueue<int> fps;
  struct ThreadStatisticsData statsData;
  volatile bool doStop;
  int captureTime;
  int sampleNumber;
  int fpsSum;
  bool dropFrameIfBufferFull;
  int deviceNumber;
  int width;
  int height;
  int fpsGoal;

protected:
  void run();

signals:
  void updateStatisticsInGUI(struct ThreadStatisticsData);
  void updateFramerate(double FPS);
};