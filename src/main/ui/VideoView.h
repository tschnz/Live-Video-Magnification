#pragma once

// Qt
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
// Local
#include "../other/Structures.h"
#include "../threads/PlayerThread.h"
#include "../threads/SavingThread.h"
#include "FrameLabel.h"
#include "MagnifyOptions.h"
namespace Ui {
class VideoView;
}

class VideoView : public QWidget {
  Q_OBJECT

public:
  explicit VideoView(QWidget *parent, QString filepath);
  ~VideoView();
  bool loadVideo(int threadPrio, int width, int height, double fps);
  void setCodec(int codec);
  void set_useVideoCodec(bool use);

private:
  Ui::VideoView *ui;
  QFileInfo file;
  QString filename;
  PlayerThread *playerThread;
  ImageProcessingFlags imageProcessingFlags;
  bool isFileLoaded;
  MagnifyOptions *magnifyOptionsTab;
  void stopPlayerThread();
  QString getFormattedTime(int time);
  void handleOriginalWindow(bool doEmit);
  FrameLabel *originalFrame;
  SavingThread *vidSaver;
  int codec;
  bool useVideoCodec;

public slots:
  void newMouseData(struct MouseData mouseData);
  void newMouseDataOriginalFrame(struct MouseData mouseData);
  void updateMouseCursorPosLabel();
  void updateMouseCursorPosLabelOriginalFrame();
  void endOfFrame_action();
  void endOfSaving_action();
  void updateProgressBar(int frame);

private slots:
  void updateFrame(const QImage &frame);
  void updateOriginalFrame(const QImage &frame);
  void updatePlayerThreadStats(struct ThreadStatisticsData statData);
  void handleContextMenuAction(QAction *action);
  void play();
  void stop();
  void pause();
  void setTime();
  void hideSettings();
  void save_action();
  void handleTabChange(int index);

signals:
  void
  newImageProcessingFlags(struct ImageProcessingFlags imageProcessingFlags);
  void setROI(QRect roi);
};