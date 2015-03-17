/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->SavingThread.h                                     */
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

#ifndef VIDEOSAVER_H
#define VIDEOSAVER_H

// Qt
#include <QtCore/QThread>
#include <QMutex>
// OpenCV
#include <opencv2/highgui/highgui.hpp>
// Local
#include "main/magnification/Magnificator.h"
#include "main/other/Structures.h"

using namespace cv;

class SavingThread : public QThread
{
    Q_OBJECT
public:
    SavingThread();
    ~SavingThread();
    void stop();
    bool loadFile(std::string source);
    void settings(ImageProcessingFlags imageProcFlags, ImageProcessingSettings imageProcSettings);
    bool saveFile(std::string destination, double framerate, QRect dimensions, bool saveOriginal);
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

#endif // VIDEOSAVER_H
