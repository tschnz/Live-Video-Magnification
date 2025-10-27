#pragma once

// Qt
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
// Local
#include "../helper/SharedImageBuffer.h"
#include "../other/Structures.h"
#include "../threads/CaptureThread.h"
#include "../threads/ProcessingThread.h"
#include "FrameLabel.h"
#include "MagnifyOptions.h"

namespace Ui {
class CameraView;
}

class CameraView : public QWidget {
  Q_OBJECT

public:
  explicit CameraView(QWidget *parent, int deviceNumber,
                      SharedImageBuffer *sharedImageBuffer);
  ~CameraView();
  bool connectToCamera(bool dropFrame, int capThreadPrio, int procThreadPrio,
                       int width, int height, int fps);
  void setCodec(int codec);

private:
  Ui::CameraView *ui = nullptr;
  ProcessingThread *processingThread = nullptr;
  CaptureThread *captureThread = nullptr;
  SharedImageBuffer *sharedImageBuffer = nullptr;
  ImageProcessingFlags imageProcessingFlags;
  void stopCaptureThread();
  void stopProcessingThread();
  int deviceNumber;
  bool isCameraConnected;
  MagnifyOptions *magnifyOptionsTab = nullptr;
  FrameLabel *originalFrame = nullptr;
  void handleOriginalWindow(bool doEmit);
  QString getFormattedTime(int timeInMSeconds);
  int codec;

public slots:
  void newMouseData(struct MouseData mouseData);
  void newMouseDataOriginalFrame(struct MouseData mouseData);
  void updateMouseCursorPosLabel();
  void updateMouseCursorPosLabelOriginalFrame();
  void clearImageBuffer();
  void frameWritten(int frames);

private slots:
  void updateFrame(const QImage &frame);
  void updateOriginalFrame(const QImage &frame);
  void updateProcessingThreadStats(struct ThreadStatisticsData statData);
  void updateCaptureThreadStats(struct ThreadStatisticsData statData);
  void handleContextMenuAction(QAction *action);
  void hideSettings();
  void record();
  void selectButton_action();
  void handleTabChange(int index);

signals:
  void
  newImageProcessingFlags(struct ImageProcessingFlags imageProcessingFlags);
  void setROI(QRect roi);
};