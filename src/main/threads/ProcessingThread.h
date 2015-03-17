/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->ProcessingThread.h                                 */
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

#ifndef PROCESSINGTHREAD_H
#define PROCESSINGTHREAD_H

// Qt
#include <QtCore/QThread>
#include <QtCore/QTime>
#include <QtCore/QQueue>
#include "QDebug"
// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
// Local
#include "main/other/Structures.h"
#include "main/other/Config.h"
#include "main/other/Buffer.h"
#include "main/helper/MatToQImage.h"
#include "main/helper/SharedImageBuffer.h"
#include "main/magnification/Magnificator.h"

using namespace cv;

class ProcessingThread : public QThread
{
    Q_OBJECT

    public:
        ProcessingThread(SharedImageBuffer *sharedImageBuffer, int deviceNumber);
        ~ProcessingThread();
        bool releaseCapture();
        QRect getCurrentROI();
        void stop();
        void getOriginalFrame(bool doEmit);
        bool startRecord(std::string filepath, bool captureOriginal);
        void stopRecord();
        bool isRecording();
        int getFPS();
        int getRecordFPS();
        int savingCodec;

    private:
        void updateFPS(int);
        bool processingBufferFilled();
        void fillProcessingBuffer();
        Magnificator magnificator;
        SharedImageBuffer *sharedImageBuffer;
        Mat currentFrame;
        Mat combinedFrame;
        Mat originalFrame;
        Rect currentROI;
        QImage frame;
        QTime t;
        QQueue<int> fps;
        QMutex doStopMutex;
        QMutex processingMutex;
        Size frameSize;
        std::vector<Mat> processingBuffer;
        int processingBufferLength;
        Point framePoint;
        struct ImageProcessingFlags imgProcFlags;
        struct ImageProcessingSettings imgProcSettings;
        struct ThreadStatisticsData statsData;
        volatile bool doStop;
        int processingTime;
        int fpsSum;
        int sampleNumber;
        int deviceNumber;
        bool emitOriginal;
        bool doRecord;
        VideoWriter output;
        int framesWritten;
        int recordingFramerate;
        bool captureOriginal;
        Mat combineFrames(Mat &frame1, Mat &frame2);
        QMutex recordMutex;

    protected:
        void run();

    public slots:
        void updateImageProcessingFlags(struct ImageProcessingFlags);
        void updateImageProcessingSettings(struct ImageProcessingSettings);
        void setROI(QRect roi);
        void updateFramerate(double fps);

    signals:
        void newFrame(const QImage &frame);
        void origFrame(const QImage &frame);
        void updateStatisticsInGUI(struct ThreadStatisticsData);
        void frameWritten(int frames);
        void maxLevels(int levels);
};

#endif // PROCESSINGTHREAD_H
