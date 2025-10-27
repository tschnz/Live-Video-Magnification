#pragma once

// Qt
#include <QMutex>
#include <QtCore/QThread>
// OpenCV
#include <opencv2/opencv.hpp>
// Local
#include "../magnification/Magnificator.h"
#include "../other/Structures.h"

using namespace cv;

class SavingThread : public QThread {
  Q_OBJECT
public:
  SavingThread();
  ~SavingThread();
  void stop();
  bool loadFile(std::string source);
  void settings(ImageProcessingFlags imageProcFlags,
                ImageProcessingSettings imageProcSettings);
  bool saveFile(std::string destination, double framerate, QRect dimensions,
                bool saveOriginal);
  bool isSaving();
  int getVideoLength();
  int getVideoCodec();
  int savingCodec;

private:
  // Thread
  bool doStop;
  QMutex doStopMutex;
  QMutex processingMutex;
  void releaseFile();
  void resetSaver();
  // Capture
  VideoCapture cap;
  int videoLength;
  std::vector<Mat> processingBuffer;
  std::vector<Mat> originalBuffer;
  int processingBufferLength;
  Rect ROI;
  Mat grabbedFrame;
  Mat currentFrame;
  bool processingBufferFilled();
  // Write
  VideoWriter out;
  Mat processedFrame;
  Mat mergedFrame;
  int getCurrentCaptureIndex();
  bool captureOriginal;
  int currentWriteIndex;
  int getCurrentReadIndex();
  Mat combineFrames(Mat &frame1, Mat &frame2);
  // Magnify
  Magnificator magnificator;
  ImageProcessingFlags imgProcFlags;
  ImageProcessingSettings imgProcSettings;

protected:
  void run();

signals:
  void endOfSaving();
  void updateProgress(int frame);
};