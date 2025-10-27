#include "CaptureThread.h"
#include <opencv2/videoio.hpp>

CaptureThread::CaptureThread(SharedImageBuffer *sharedImageBuffer,
                             int deviceNumber, bool dropFrameIfBufferFull,
                             int width, int height, int fpsLimit)
    : QThread(), sharedImageBuffer(sharedImageBuffer) {
  // Save passed parameters
  this->dropFrameIfBufferFull = dropFrameIfBufferFull;
  this->deviceNumber = deviceNumber;
  this->width = width;
  this->height = height;
  this->fpsGoal = fpsLimit;
  // Initialize variables(s)
  doStop = false;
  sampleNumber = 0;
  fpsSum = 0;
  fps.clear();
  statsData.averageFPS = 0;
  statsData.nFramesProcessed = 0;
}

void CaptureThread::run() {
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

    // Save capture time
    captureTime = t.elapsed();
    // Start timer (used to calculate capture rate)
    t.start();

    // Capture frame (if available)
    if (!cap.grab())
      continue;
    // Retrieve frame
    cap.retrieve(grabbedFrame);

    // Add frame to buffer
    sharedImageBuffer->getByDeviceNumber(deviceNumber)
        ->add(grabbedFrame, dropFrameIfBufferFull);

    // Update statistics
    updateFPS(captureTime);
    statsData.nFramesProcessed++;
    // Inform GUI of updated statistics
    emit updateStatisticsInGUI(statsData);
  }
  qDebug() << "Stopping capture thread...";
}

bool CaptureThread::connectToCamera() {
  // Open camera
  bool camOpenResult = cap.open(deviceNumber, cv::CAP_ANY);

  if(!cap.isOpened())
      return false;

  // Set resolution
  if (width != -1)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
  if (height != -1)
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
  // Set maximum frames per second
  if (fpsGoal != -1)
    cap.set(cv::CAP_PROP_FPS, fpsGoal);
  // Return result
  return camOpenResult;
}

bool CaptureThread::disconnectCamera() {
  // Camera is connected
  if (cap.isOpened()) {
    // Disconnect camera
    cap.release();
    return true;
  }
  // Camera is NOT connected
  else
    return false;
}

void CaptureThread::updateFPS(int timeElapsed) {
  // Add instantaneous FPS value to queue
  if (timeElapsed > 0) {
    fps.enqueue((int)1000 / timeElapsed);
    // Increment sample number
    sampleNumber++;
  }
  // Maximum size of queue is DEFAULT_CAPTURE_FPS_STAT_QUEUE_LENGTH
  if (fps.size() > CAPTURE_FPS_STAT_QUEUE_LENGTH)
    fps.dequeue();
  // Update FPS value every DEFAULT_CAPTURE_FPS_STAT_QUEUE_LENGTH samples
  if ((fps.size() == CAPTURE_FPS_STAT_QUEUE_LENGTH) &&
      (sampleNumber == CAPTURE_FPS_STAT_QUEUE_LENGTH)) {
    // Empty queue and store sum
    while (!fps.empty())
      fpsSum += fps.dequeue();
    // Calculate average FPS
    int newFramerate = fpsSum / CAPTURE_FPS_STAT_QUEUE_LENGTH;
    // Check if new fps rate is different, inform processing thread if it is
    if (newFramerate < statsData.averageFPS - 1 ||
        newFramerate > statsData.averageFPS + 1)
      emit updateFramerate(newFramerate);

    statsData.averageFPS = newFramerate;
    // Reset sum
    fpsSum = 0;
    // Reset sample number
    sampleNumber = 0;
  }
}

void CaptureThread::stop() {
  QMutexLocker locker(&doStopMutex);
  doStop = true;
}

bool CaptureThread::isCameraConnected() { return cap.isOpened(); }

int CaptureThread::getInputSourceWidth() {
  return cap.get(cv::CAP_PROP_FRAME_WIDTH);
}

int CaptureThread::getInputSourceHeight() {
  return cap.get(cv::CAP_PROP_FRAME_HEIGHT);
}
