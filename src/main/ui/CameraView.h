/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->CameraView.h                                       */
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

#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

// Qt
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
// Local
#include "main/threads/CaptureThread.h"
#include "main/threads/ProcessingThread.h"
#include "main/other/Structures.h"
#include "main/helper/SharedImageBuffer.h"
#include "main/ui/MagnifyOptions.h"
#include "main/ui/FrameLabel.h"

namespace Ui {
    class CameraView;
}

class CameraView : public QWidget
{
    Q_OBJECT

    public:
        explicit CameraView(QWidget *parent, int deviceNumber, SharedImageBuffer *sharedImageBuffer);
        ~CameraView();
        bool connectToCamera(bool dropFrame, int capThreadPrio, int procThreadPrio, int width, int height, int fps);
        void setCodec(int codec);

    private:
        Ui::CameraView *ui;
        ProcessingThread *processingThread;
        CaptureThread *captureThread;
        SharedImageBuffer *sharedImageBuffer;
        ImageProcessingFlags imageProcessingFlags;
        void stopCaptureThread();
        void stopProcessingThread();
        int deviceNumber;
        bool isCameraConnected;
        MagnifyOptions *magnifyOptionsTab;
        FrameLabel *originalFrame;
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
        void newImageProcessingFlags(struct ImageProcessingFlags imageProcessingFlags);
        void setROI(QRect roi);
};

#endif // CAMERAVIEW_H
