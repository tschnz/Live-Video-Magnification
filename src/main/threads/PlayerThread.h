#pragma once

// C++
#include <cmath>
// Qt
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QtCore/QElapsedTimer>
#include <QtCore/QQueue>
#include <QtCore/QThread>
// OpenCV
#include <opencv2/opencv.hpp>
// Local
#include "../helper/MatToQImage.h"
#include "../magnification/Magnificator.h"
#include "../other/Config.h"
#include "../other/Structures.h"

using namespace cv;

class PlayerThread : public QThread {
  Q_OBJECT

public:
  PlayerThread(const std::string filepath, int width, int height, double fps);
  ~PlayerThread();
  void stop();
  // Capture
  bool loadFile();
  bool releaseFile();
  bool isFileLoaded();
  int getInputSourceWidth();
  int getInputSourceHeight();
  // Process
  QRect getCurrentROI();
  // Player
  bool isStopping();
  bool isPausing();
  bool isPlaying();
  void setCurrentFrame(int framenumber);
  void playAction();
  void stopAction();
  void pauseAction();
  void setCurrentTime(int ms);
  double getCurrentFramenumber();
  double getCurrentPosition();
  double getInputFrameLength();
  double getInputTimeLength();
  double getFPS();
  void getOriginalFrame(bool doEmit);

private:
  QMutex doStopMutex;
  struct ThreadStatisticsData statsData;
  double lengthInMs;
  double lengthInFrames;
  const std::string filepath;
  int getCurrentReadIndex();
  // Capture
  VideoCapture cap;
  Mat grabbedFrame;
  int playedTime;
  int width;
  int height;
  std::vector<Mat> originalBuffer;
  void setBufferSize();
  // Process
  // This is the current Framenr that is grabbed from magnificator is always
  // < processingBufferLength, used to process videos rightly from beginning of
  // vid
  int currentWriteIndex;
  double fps;
  void updateFPS(int timeElapsed);
  Mat currentFrame;
  Rect currentROI;
  QImage frame;
  QImage originalFrame;
  // processing measurement
  QElapsedTimer t;
  int processingTime;
  int fpsSum;
  int sampleNumber;
  QQueue<int> fpsQueue;
  QMutex processingMutex;
  Size frameSize;
  Point framePoint;
  struct ImageProcessingFlags imgProcFlags;
  struct ImageProcessingSettings imgProcSettings;
  // Player variables
  volatile bool doStop;
  volatile bool doPause;
  volatile bool doPlay;
  bool emitOriginal;
  void endOfFrame_action();
  // Magnifying
  bool processingBufferFilled();
  void fillProcessingBuffer();
  Magnificator magnificator;
  std::vector<Mat> processingBuffer;
  int processingBufferLength;

protected:
  void run();

private slots:
  void updateImageProcessingFlags(struct ImageProcessingFlags);
  void updateImageProcessingSettings(struct ImageProcessingSettings);
  void setROI(QRect roi);
  void pauseThread();

signals:
  void updateStatisticsInGUI(struct ThreadStatisticsData);
  void newFrame(const QImage &frame);
  void origFrame(const QImage &frame);
  void endOfFrame();
  void maxLevels(int levels);
};