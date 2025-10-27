#pragma once

// Qt
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QWaitCondition>
// OpenCV
#include <opencv2/opencv.hpp>
// Local
#include <main/other/Buffer.h>

using namespace cv;

class SharedImageBuffer {
public:
  SharedImageBuffer();
  void add(int deviceNumber, Buffer<Mat> *imageBuffer);
  Buffer<Mat> *getByDeviceNumber(int deviceNumber);
  void removeByDeviceNumber(int deviceNumber);
  void wakeAll();
  bool containsImageBufferForDeviceNumber(int deviceNumber);

private:
  QHash<int, Buffer<Mat> *> imageBufferMap;
  QWaitCondition wc;
  QMutex mutex;
  int nArrived;
};