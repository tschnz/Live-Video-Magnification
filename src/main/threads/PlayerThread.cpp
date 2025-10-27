#include "PlayerThread.h"

// Constructor
PlayerThread::PlayerThread(const std::string filepath, int width, int height,
                           double fps)
    : QThread(), filepath(filepath), width(width), height(height), fps(fps),
      emitOriginal(false) {
  doStop = true;
  doPause = false;
  doPlay = false;
  statsData.averageFPS = 0;
  statsData.averageVidProcessingFPS = 0;
  statsData.nFramesProcessed = 0;

  originalBuffer = std::vector<Mat>();
  processingBuffer = std::vector<Mat>();

  imgProcSettings.framerate = fps;

  sampleNumber = 0;
  fpsSum = 0;
  fpsQueue.clear();

  this->magnificator =
      Magnificator(&processingBuffer, &imgProcFlags, &imgProcSettings);
  this->cap = VideoCapture();
  currentWriteIndex = 0;
}

// Destructor
PlayerThread::~PlayerThread() {
  doStopMutex.lock();
  doStop = true;
  if (releaseFile())
    qDebug() << "Released File.";
  doStopMutex.unlock();
  wait();
}

// Thread
void PlayerThread::run() {
  // The standard delay time to keep FPS playing rate without processing time
  double delay = 1000.0 / fps;
  qDebug() << "Starting player thread...";
  QElapsedTimer mTime;

  /////////////////////////////////////
  /// Stop thread if doStop=TRUE /////
  ///////////////////////////////////
  while (!doStop) {
    //////////////////////////////////////////////
    /// Stop thread if processed all frames /////
    ////////////////////////////////////////////
    if (currentWriteIndex >= lengthInFrames) {
      endOfFrame_action();
      break;
    }

    // Save process time
    processingTime = t.elapsed();
    // Start timer
    t.start();

    /////////////////////////////////////
    /// Pause thread if doPause=TRUE ///
    ///////////////////////////////////
    doStopMutex.lock();
    if (doPause) {
      doPlay = false;
      doStopMutex.unlock();
      break;
    }
    doStopMutex.unlock();

    // Start timer and capture time needed to process 1 frame here
    if (getCurrentReadIndex() == processingBufferLength - 1)
      mTime.start();
    // Switch to process images on the fly instead of processing a whole buffer,
    // reducing MEM
    if (imgProcFlags.colorMagnifyOn && processingBufferLength > 2 &&
        magnificator.getBufferSize() > 2) {
      processingBufferLength = 2;
    }

    ///////////////////////////////////
    /////////// Capturing ////////////
    /////////////////////////////////
    // Fill buffer, check if it's the start of magnification or not
    for (int i = processingBuffer.size();
         i < processingBufferLength && getCurrentFramenumber() < lengthInFrames;
         i++) {
      processingMutex.lock();

      // Try to grab the next Frame
      if (cap.read(grabbedFrame)) {
        // Preprocessing
        // Set ROI of frame
        currentFrame = Mat(grabbedFrame.clone(), currentROI);
        // Convert to grayscale
        if (imgProcFlags.grayscaleOn &&
            (currentFrame.channels() == 3 || currentFrame.channels() == 4)) {
          cvtColor(currentFrame, currentFrame, cv::COLOR_BGR2GRAY, 1);
        }

        // Fill fuffer
        processingBuffer.push_back(currentFrame);
        if (emitOriginal)
          originalBuffer.push_back(currentFrame.clone());
      }

      // Wasn't able to grab frame, abort thread
      else {
        processingMutex.unlock();
        endOfFrame_action();
        break;
      }

      processingMutex.unlock();
    }
    // Breakpoint if grabbing frames wasn't succesful
    if (doStop) {
      break;
    }

    ///////////////////////////////////
    /////////// Magnifying ///////////
    /////////////////////////////////
    processingMutex.lock();

    if (imgProcFlags.colorMagnifyOn) {
      magnificator.colorMagnify();
      if (magnificator.hasFrame()) {
        currentFrame = magnificator.getFrameFirst();
      }
    } else if (imgProcFlags.laplaceMagnifyOn) {
      magnificator.laplaceMagnify();
      if (magnificator.hasFrame()) {
        currentFrame = magnificator.getFrameFirst();
      }
    } else if (imgProcFlags.rieszMagnifyOn) {
      magnificator.rieszMagnify();
      if (magnificator.hasFrame()) {
        currentFrame = magnificator.getFrameFirst();
      }
    } else {
      // Read frames unmagnified
      currentFrame = processingBuffer.at(getCurrentReadIndex());
      // Erase to keep buffer size
      processingBuffer.erase(processingBuffer.begin());
    }
    // Increase number of frames given to GUI
    currentWriteIndex++;

    frame = MatToQImage(currentFrame);
    if (emitOriginal) {
      originalFrame = MatToQImage(originalBuffer.front());
      if (!originalBuffer.empty())
        originalBuffer.erase(originalBuffer.begin());
    }

    processingMutex.unlock();

    ///////////////////////////////////
    /////////// Updating /////////////
    /////////////////////////////////
    // Inform GUI thread of new frame
    emit newFrame(frame);
    // Inform GUI thread of original frame if option was set
    if (emitOriginal)
      emit origFrame(originalFrame);

    // Update statistics
    updateFPS(processingTime);
    statsData.nFramesProcessed = currentWriteIndex;
    // Inform GUI about updatet statistics
    emit updateStatisticsInGUI(statsData);

    // To keep FPS playing rate, adjust waiting time, dependent on the time that
    // is used to process one frame
    double diff = mTime.elapsed();
    int wait = max(delay - diff, 0.0);
    this->msleep(wait);
  }
  qDebug() << "Stopping player thread...";
}

int PlayerThread::getCurrentReadIndex() {
  return std::min(currentWriteIndex, processingBufferLength - 1);
}

double PlayerThread::getFPS() { return fps; }

void PlayerThread::getOriginalFrame(bool doEmit) {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);

  originalBuffer.clear();
  originalBuffer = processingBuffer;
  emitOriginal = doEmit;
}

// Load videofile
bool PlayerThread::loadFile() {
  // Just in case, release file
  releaseFile();
  cap = VideoCapture(filepath);

  // Open file
  bool openResult = isFileLoaded();
  if (!openResult)
    openResult = cap.open(filepath, cv::CAP_FFMPEG);

  // Set resolution
  if (width != -1)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
  if (height != -1)
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
  if (fps == -1) {
    fps = cap.get(cv::CAP_PROP_FPS);
  }

  // OpenCV can't read all mp4s properly, fps is often false
  if (fps < 0 || !std::isfinite(fps)) {
    fps = 30;
  }

  // initialize Buffer length
  setBufferSize();

  // Write information in Settings
  statsData.averageFPS = fps;
  imgProcSettings.framerate = fps;
  imgProcSettings.frameHeight = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  imgProcSettings.frameWidth = cap.get(cv::CAP_PROP_FRAME_WIDTH);

  // Save total length of video
  lengthInFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);

  return openResult;
}

// Release the file from VideoCapture
bool PlayerThread::releaseFile() {
  // File is loaded
  if (cap.isOpened()) {
    // Release File
    cap.release();
    return true;
  }
  // File is NOT laoded
  else
    return false;
}

// Stop the thread
void PlayerThread::stop() {
  doStop = true;
  doPause = false;
  doPlay = false;

  setBufferSize();
  releaseFile();

  currentWriteIndex = 0;
}

void PlayerThread::endOfFrame_action() {
  doStop = true;
  stop();
  emit endOfFrame();
  statsData.nFramesProcessed = 0;
  emit updateStatisticsInGUI(statsData);
}

bool PlayerThread::isFileLoaded() { return cap.isOpened(); }

int PlayerThread::getInputSourceWidth() { return imgProcSettings.frameWidth; }

int PlayerThread::getInputSourceHeight() { return imgProcSettings.frameHeight; }

void PlayerThread::setROI(QRect roi) {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);
  currentROI.x = roi.x();
  currentROI.y = roi.y();
  currentROI.width = roi.width();
  currentROI.height = roi.height();
  int levels = magnificator.calculateMaxLevels(roi);
  magnificator.clearBuffer();
  locker1.unlock();
  locker2.unlock();
  setBufferSize();
  emit maxLevels(levels);
}

QRect PlayerThread::getCurrentROI() {
  return QRect(currentROI.x, currentROI.y, currentROI.width, currentROI.height);
}

// Private Slots
void PlayerThread::updateImageProcessingFlags(
    struct ImageProcessingFlags imgProcessingFlags) {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);
  this->imgProcFlags.grayscaleOn = imgProcessingFlags.grayscaleOn;
  this->imgProcFlags.colorMagnifyOn = imgProcessingFlags.colorMagnifyOn;
  this->imgProcFlags.laplaceMagnifyOn = imgProcessingFlags.laplaceMagnifyOn;
  this->imgProcFlags.rieszMagnifyOn = imgProcessingFlags.rieszMagnifyOn;
  locker1.unlock();
  locker2.unlock();

  setBufferSize();
}

void PlayerThread::updateImageProcessingSettings(
    struct ImageProcessingSettings imgProcessingSettings) {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);
  bool resetBuffer =
      (this->imgProcSettings.levels != imgProcessingSettings.levels);

  this->imgProcSettings.amplification = imgProcessingSettings.amplification;
  this->imgProcSettings.coWavelength = imgProcessingSettings.coWavelength;
  this->imgProcSettings.coLow = imgProcessingSettings.coLow;
  this->imgProcSettings.coHigh = imgProcessingSettings.coHigh;
  this->imgProcSettings.chromAttenuation =
      imgProcessingSettings.chromAttenuation;
  this->imgProcSettings.levels = imgProcessingSettings.levels;

  if (resetBuffer) {
    locker1.unlock();
    locker2.unlock();
    setBufferSize();
  }
}

// Public Slots / Video control
void PlayerThread::playAction() {
  if (!isPlaying()) {

    if (!cap.isOpened())
      loadFile();

    if (isPausing()) {
      doStop = false;
      doPause = false;
      doPlay = true;
      cap.set(cv::CAP_PROP_POS_FRAMES, getCurrentFramenumber());
      start();
    } else if (isStopping()) {
      doStop = false;
      doPause = false;
      doPlay = true;
      start();
    }
  }
}
void PlayerThread::stopAction() { stop(); }
void PlayerThread::pauseAction() {
  doStop = false;
  doPause = true;
  doPlay = false;
}

void PlayerThread::pauseThread() { pauseAction(); }

bool PlayerThread::isPlaying() { return this->doPlay; }

bool PlayerThread::isStopping() { return this->doStop; }

bool PlayerThread::isPausing() { return this->doPause; }

void PlayerThread::setCurrentFrame(int framenumber) {
  currentWriteIndex = framenumber;
  setBufferSize();
}

void PlayerThread::setCurrentTime(int ms) {
  if (cap.isOpened())
    cap.set(cv::CAP_PROP_POS_MSEC, ms);
}

double PlayerThread::getInputFrameLength() { return lengthInFrames; }

double PlayerThread::getInputTimeLength() { return lengthInMs; }

double PlayerThread::getCurrentFramenumber() {
  return cap.get(cv::CAP_PROP_POS_FRAMES);
}

double PlayerThread::getCurrentPosition() {
  return cap.get(cv::CAP_PROP_POS_MSEC);
}

void PlayerThread::updateFPS(int timeElapsed) {
  // Add instantaneous FPS value to queue
  if (timeElapsed > 0) {
    fpsQueue.enqueue((int)1000 / timeElapsed);
    // Increment sample number
    sampleNumber++;
  }

  // Maximum size of queue is DEFAULT_PROCESSING_FPS_STAT_QUEUE_LENGTH
  if (fpsQueue.size() > PROCESSING_FPS_STAT_QUEUE_LENGTH)
    fpsQueue.dequeue();

  // Update FPS value every DEFAULT_PROCESSING_FPS_STAT_QUEUE_LENGTH samples
  if ((fpsQueue.size() == PROCESSING_FPS_STAT_QUEUE_LENGTH) &&
      (sampleNumber == PROCESSING_FPS_STAT_QUEUE_LENGTH)) {
    // Empty queue and store sum
    while (!fpsQueue.empty())
      fpsSum += fpsQueue.dequeue();
    // Calculate average FPS
    statsData.averageVidProcessingFPS =
        fpsSum / PROCESSING_FPS_STAT_QUEUE_LENGTH;
    // Reset sum
    fpsSum = 0;
    // Reset sample number
    sampleNumber = 0;
  }
}

// Magnificator
void PlayerThread::fillProcessingBuffer() {
  processingBuffer.push_back(currentFrame);
}

bool PlayerThread::processingBufferFilled() {
  return (processingBuffer.size() == processingBufferLength);
}

void PlayerThread::setBufferSize() {
  QMutexLocker locker1(&doStopMutex);
  QMutexLocker locker2(&processingMutex);

  processingBuffer.clear();
  originalBuffer.clear();
  magnificator.clearBuffer();

  if (imgProcFlags.colorMagnifyOn) {
    processingBufferLength =
        magnificator.getOptimalBufferSize(imgProcSettings.framerate);
  } else if (imgProcFlags.laplaceMagnifyOn) {
    processingBufferLength = 2;
  } else if (imgProcFlags.rieszMagnifyOn) {
    processingBufferLength = 2;
  } else {
    processingBufferLength = 1;
  }

  if (cap.isOpened() || !doStop)
    cap.set(cv::CAP_PROP_POS_FRAMES,
            std::max(currentWriteIndex - processingBufferLength, 0));
}
