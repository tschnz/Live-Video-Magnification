#include "SavingThread.h"
#include <opencv2/core/hal/interface.h>

// Constructor
SavingThread::SavingThread() : QThread() {
  this->doStop = true;

  currentWriteIndex = 0;
  processingBufferLength = 1;

  cap = VideoCapture();
  out = VideoWriter();
}
// Destructor
SavingThread::~SavingThread() {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);

  doStop = true;
  releaseFile();
  wait();
}

// Thread. Is designed to run till completed, then shut itself down
void SavingThread::run() {
  qDebug() << "Starting SavingThread thread";
  while (1) {
    ////////////////////////// ///////
    // Stop thread if doStop=TRUE //
    ////////////////////////// ///////
    doStopMutex.lock();
    if (doStop) {
      doStop = false;
      doStopMutex.unlock();
      break;
    }
    doStopMutex.unlock();
    ////////////////////////// ////////
    ////////////////////////// ////////
    // if at last frame, skip grabbing frames until we have processed every
    // frame
    if (getCurrentCaptureIndex() < videoLength) {

      if (imgProcFlags.colorMagnifyOn && processingBufferLength > 2 &&
          currentWriteIndex == 1) {
        processingBufferLength = 2;
      }

      for (int i = processingBuffer.size(); i < processingBufferLength; i++) {
        // Try to read the Frame
        if (cap.read(grabbedFrame)) {
          // Clone the most recent frame
          currentFrame = Mat(grabbedFrame.clone(), ROI);

          // Do the PREPROCESSING
          if (imgProcFlags.grayscaleOn &&
              (currentFrame.channels() == 3 || currentFrame.channels() == 4)) {
            cvtColor(currentFrame, currentFrame, cv::COLOR_BGR2GRAY, 1);
          }

          // Fill Buffer
          processingBuffer.push_back(currentFrame);

          // If capturing original, push back, too
          if (captureOriginal)
            originalBuffer.push_back(currentFrame);
        } else {
          doStop = true;
          break;
        }
      }

      if (doStop)
        continue;
    }
    processingMutex.lock();
    /// Process

    if (imgProcFlags.colorMagnifyOn) {
      magnificator.colorMagnify();
      processedFrame = magnificator.getFrameFirst();
    } else if (imgProcFlags.laplaceMagnifyOn) {
      magnificator.laplaceMagnify();
      processedFrame = magnificator.getFrameFirst();
    } else if (imgProcFlags.rieszMagnifyOn) {
      magnificator.rieszMagnify();
      processedFrame = magnificator.getFrameFirst();
    } else {
      processedFrame = processingBuffer.front();
      processingBuffer.erase(processingBuffer.begin());
    }
    currentWriteIndex++;

    if(processedFrame.type() == CV_32FC1)
    {
        cv::Mat normalizedMat;
        cv::normalize(processedFrame, normalizedMat, 0, 255, cv::NORM_MINMAX);
        normalizedMat.convertTo(normalizedMat, CV_8UC1);
        processedFrame = normalizedMat;
    }
    else if(processedFrame.type() == CV_32FC3)
    {
        cv::Mat normalizedMat;
        cv::normalize(processedFrame, normalizedMat, 0, 255, cv::NORM_MINMAX);
        normalizedMat.convertTo(normalizedMat, CV_8UC3);
        processedFrame = normalizedMat;
    }


    // Combine Frames
    if (captureOriginal) {
      Mat originalFrame = originalBuffer.front();
      mergedFrame = combineFrames(processedFrame, originalFrame);
      originalBuffer.erase(originalBuffer.begin());
    }

    processingMutex.unlock();

    /// Record
    if (!doStop && out.isOpened()) {
      if (captureOriginal)
        out.write(mergedFrame);
      else
        out.write(processedFrame);
    }

    // Inform VideoView about saving progress
    emit updateProgress(currentWriteIndex);

    // Stop Thread if video is fully processed
    if (currentWriteIndex >= videoLength)
      doStop = true;
  }
  emit endOfSaving();
  qDebug() << "Stopping SavingThread thread";
  resetSaver();
}

void SavingThread::resetSaver() {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);

  processingBuffer.clear();
  magnificator.clearBuffer();
  currentWriteIndex = 0;
  releaseFile();
  cap.set(cv::CAP_PROP_POS_FRAMES, 0);
  doStop = true;
}

void SavingThread::stop() {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);

  doStop = true;
  releaseFile();
}

bool SavingThread::loadFile(std::string source) {
  if (cap.open(source)) {
    videoLength = cap.get(cv::CAP_PROP_FRAME_COUNT);
    return true;
  } else
    return false;
}

void SavingThread::settings(ImageProcessingFlags imageProcFlags,
                            ImageProcessingSettings imageProcSettings) {
  this->imgProcFlags = imageProcFlags;
  this->imgProcSettings = imageProcSettings;

  this->magnificator =
      Magnificator(&processingBuffer, &imgProcFlags, &imgProcSettings);
}

bool SavingThread::saveFile(std::string destination, double framerate,
                            QRect dimensions, bool captureOriginal) {
  if (imgProcFlags.colorMagnifyOn) {
    processingBufferLength = magnificator.getOptimalBufferSize(framerate);
  } else if (imgProcFlags.laplaceMagnifyOn) {
    processingBufferLength = 2;
  } else if (imgProcFlags.rieszMagnifyOn) {
    processingBufferLength = 2;
  } else
    processingBufferLength = 1;

  this->ROI = Rect(dimensions.x(), dimensions.y(), dimensions.width(),
                   dimensions.height());
  this->captureOriginal = captureOriginal;
  Size s = captureOriginal ? Size(ROI.width * 2, ROI.height)
                           : Size(ROI.width, ROI.height);
  // Codec WATCH OUT: Not every codec is available on every PC,
  // MP4V was chosen because it's famous among various systems
  // int codec = CV_FOURCC('M','P','4','V');

  bool success = (out.open(destination, savingCodec, framerate, s,
                           !(imgProcFlags.grayscaleOn)));
  // Update the settings, to add framerate
  imgProcSettings.framerate = framerate;
  // If succesful, indicate thread is running
  if (success)
    doStop = false;

  return success;
}

void SavingThread::releaseFile() {
  if (cap.isOpened())
    cap.release();
  if (out.isOpened())
    out.release();
}

// Return the current Position of cap
int SavingThread::getCurrentCaptureIndex() {
  return cap.get(cv::CAP_PROP_POS_FRAMES);
}

bool SavingThread::processingBufferFilled() {
  return (processingBuffer.size() == processingBufferLength);
}

int SavingThread::getCurrentReadIndex() {
  return std::min(currentWriteIndex, processingBufferLength - 1);
}

// Combine Frames into one Frame, depending on their size
Mat SavingThread::combineFrames(Mat &frame1, Mat &frame2) {
  Mat roi;
  int w = (int)ROI.width;
  int h = (int)ROI.height;

  Mat mergedFrame = Mat(Size(w * 2, h), frame1.type());
  roi = Mat(mergedFrame, Rect(0, 0, w, h));
  frame1.copyTo(roi);
  roi = Mat(mergedFrame, Rect(w, 0, w, h));
  frame2.copyTo(roi);

  return mergedFrame;
}

bool SavingThread::isSaving() {
  QMutexLocker locker(&doStopMutex);
  return !doStop;
}

int SavingThread::getVideoLength() { return videoLength; }

int SavingThread::getVideoCodec() {
  int codec = 0;
  if (cap.isOpened())
    codec = cap.get(cv::CAP_PROP_FOURCC);

  return codec;
}
