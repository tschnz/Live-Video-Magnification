/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->VideoView.h                                        */
/*                                                                                  */
/* This program is free software: you can redistribute it and/or modify             */
/* it under the terms of the GNU General Public License as published by             */
/* the Free Software Foundation, either version 3 of the License, or                */
/* (at your option) any later version.                                              */
/*                                                                                  */
/* This program is distributed in the hope that it will be useful,                  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    */
/* GNU General Public License for more details.                                     */
/*                                                                                  */
/* You should have received a copy of the GNU General Public License                */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.            */
/************************************************************************************/

#ifndef VIDEOVIEW_H
#define VIDEOVIEW_H

// Qt
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
// Local
#include "main/ui/MagnifyOptions.h"
#include "main/other/Structures.h"
#include "main/threads/PlayerThread.h"
#include "main/ui/FrameLabel.h"
#include "main/threads/SavingThread.h"
namespace Ui {
    class VideoView;
}

class VideoView : public QWidget
{
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
    void newImageProcessingFlags(struct ImageProcessingFlags imageProcessingFlags);
    void setROI(QRect roi);
};

#endif // VIDEOVIEW_H
