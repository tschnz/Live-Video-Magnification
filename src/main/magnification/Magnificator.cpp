#include "Magnificator.h"

#define _USE_MATH_DEFINES
#include <cmath>

////////////////////////
/// Constructor /////////
////////////////////////
Magnificator::Magnificator(std::vector<Mat> *pBuffer,
                           ImageProcessingFlags *imageProcFlags,
                           ImageProcessingSettings *imageProcSettings)
    : processingBuffer(pBuffer), imgProcFlags(imageProcFlags),
      imgProcSettings(imageProcSettings), currentFrame(0) {
  levels = 4;
  exaggeration_factor = 2.f;
  lambda = 0;
  delta = 0;
}
Magnificator::~Magnificator() { clearBuffer(); }

int Magnificator::calculateMaxLevels() {
  Size s = processingBuffer->front().size();
  return calculateMaxLevels(s);
}
int Magnificator::calculateMaxLevels(QRect r) {
  Size s = Size(r.width(), r.height());
  return calculateMaxLevels(s);
}
int Magnificator::calculateMaxLevels(Size s) {
  if (s.width > 5 && s.height > 5) {
    const cv::Size halved((1 + s.width) / 2, (1 + s.height) / 2);
    return 1 + calculateMaxLevels(halved);
  }
  return 0;
}

////////////////////////
/// Magnification ///////
////////////////////////
void Magnificator::colorMagnify() {
  int pBufferElements = processingBuffer->size();
  // Magnify only when processing buffer holds new images
  if (currentFrame >= pBufferElements)
    return;
  // Number of levels in pyramid
  // levels = DEFAULT_COL_MAG_LEVELS;
  levels = imgProcSettings->levels;
  Mat input, output, color, filteredFrame, downSampledFrame, filteredMat;
  std::vector<Mat> inputFrames, inputPyramid;

  int offset = 0;
  int pChannels;

  // Process every frame in buffer that wasn't magnified yet
  while (currentFrame < pBufferElements) {
    // Grab oldest frame from processingBuffer and delete it to save memory
    input = processingBuffer->front().clone();
    processingBuffer->erase(processingBuffer->begin());

    // Convert input image to 32bit float
    pChannels = input.channels();
    if (!(imgProcFlags->grayscaleOn || pChannels <= 2))
      input.convertTo(input, CV_32FC1);
    else
      input.convertTo(input, CV_32FC3);

    // Save input frame to add motion later
    inputFrames.push_back(input);

    /* 1. SPATIAL FILTER, BUILD GAUSS PYRAMID */
    buildGaussPyrFromImg(input, levels, inputPyramid);

    /* 2. CONCAT EVERY SMALLEST FRAME FROM PYRAMID IN ONE LARGE MAT, 1COL =
     * 1FRAME */
    downSampledFrame = inputPyramid.at(levels - 1);
    img2tempMat(downSampledFrame, downSampledMat,
                getOptimalBufferSize(imgProcSettings->framerate));

    // Save how many frames we've currently downsampled
    ++currentFrame;
    ++offset;
  }

  /* 3. TEMPORAL FILTER */
  idealFilter(downSampledMat, filteredMat, imgProcSettings->coLow,
              imgProcSettings->coHigh, imgProcSettings->framerate);

  /* 4. AMPLIFY */
  amplifyGaussian(filteredMat, filteredMat);

  // Add amplified image (color) to every frame
  for (int i = currentFrame - offset; i < currentFrame; ++i) {

    /* 5. DE-CONCAT 1COL TO DOWNSAMPLED COLOR IMAGE */
    tempMat2img(filteredMat, i, downSampledFrame.size(), filteredFrame);

    /* 6. RECONSTRUCT COLOR IMAGE FROM PYRAMID */
    buildImgFromGaussPyr(filteredFrame, levels, color, input.size());

    /* 7. ADD COLOR IMAGE TO ORIGINAL IMAGE */
    output = inputFrames.front() + color;

    // Scale output image an convert back to 8bit unsigned
    double min, max;
    minMaxLoc(output, &min, &max);
    if (!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
      output.convertTo(output, CV_8UC1, 255.0 / (max - min),
                       -min * 255.0 / (max - min));
    } else {
      output.convertTo(output, CV_8UC3, 255.0 / (max - min),
                       -min * 255.0 / (max - min));
    }

    // Fill internal buffer with magnified image
    magnifiedBuffer.push_back(output);
    // Delete the currently processed input image
    inputFrames.erase(inputFrames.begin());
  }
}

void Magnificator::laplaceMagnify() {
  int pBufferElements = processingBuffer->size();
  // Magnify only when processing buffer holds new images
  if (currentFrame >= pBufferElements)
    return;
  // Number of levels in pyramid
  //    levels = DEFAULT_LAP_MAG_LEVELS;
  levels = imgProcSettings->levels;

  Mat input, output, motion;
  vector<Mat> inputPyramid;
  int pChannels;

  // Process every frame in buffer that wasn't magnified yet
  while (currentFrame < pBufferElements) {
    // Grab oldest frame from processingBuffer and delete it to save memory
    input = processingBuffer->front().clone();
    if (currentFrame > 0)
      processingBuffer->erase(processingBuffer->begin());

    // Convert input image to 32bit float
    pChannels = input.channels();
    if (!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
      // Convert color images to Lab
      input.convertTo(input, CV_32FC3, 1.0 / 255.0f);
      cvtColor(input, input, cv::COLOR_BGR2Lab);
    } else
      input.convertTo(input, CV_32FC1, 1.0 / 255.0f);

    /* 1. SPATIAL FILTER, BUILD LAPLACE PYRAMID */
    buildLaplacePyrFromImg(input, levels, inputPyramid);

    // If first frame ever, save unfiltered pyramid
    if (currentFrame == 0) {
      lowpassHi = inputPyramid;
      lowpassLo = inputPyramid;
      motionPyramid = inputPyramid;
    } else {
      /* 2. TEMPORAL FILTER EVERY LEVEL OF LAPLACE PYRAMID */
      for (int curLevel = 0; curLevel < levels; ++curLevel) {
        iirFilter(inputPyramid.at(curLevel), motionPyramid.at(curLevel),
                  lowpassHi.at(curLevel), lowpassLo.at(curLevel),
                  imgProcSettings->coLow, imgProcSettings->coHigh);
      }

      int w = input.size().width;
      int h = input.size().height;

      // Amplification variable
      delta = imgProcSettings->coWavelength /
              (8.0 * (1.0 + imgProcSettings->amplification));

      // Amplification Booster for better visualization
      exaggeration_factor = DEFAULT_LAP_MAG_EXAGGERATION;

      // compute representative wavelength, lambda
      // reduces for every pyramid level
      lambda = sqrt(w * w + h * h) / 3.0;

      /* 3. AMPLIFY EVERY LEVEL OF LAPLACE PYRAMID */
      for (int curLevel = levels; curLevel >= 0; --curLevel) {
        amplifyLaplacian(motionPyramid.at(curLevel), motionPyramid.at(curLevel),
                         curLevel);
        lambda /= 2.0;
      }
    }

    /* 4. RECONSTRUCT MOTION IMAGE FROM PYRAMID */
    buildImgFromLaplacePyr(motionPyramid, levels, motion);

    /* 5. ATTENUATE (if not grayscale) */
    attenuate(motion, motion);
    /* 6. ADD MOTION TO ORIGINAL IMAGE */
    if (currentFrame > 0)
      output = input + motion;
    else
      output = input;

    // Scale output image an convert back to 8bit unsigned
    if (!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
      // Convert Lab image back to BGR
      cvtColor(output, output, cv::COLOR_Lab2BGR);
      output.convertTo(output, CV_8UC3, 255.0, 1.0 / 255.0);
    } else
      output.convertTo(output, CV_8UC1, 255.0, 1.0 / 255.0);

    // Fill internal buffer with magnified image
    magnifiedBuffer.push_back(output);
    ++currentFrame;
  }
}

void Magnificator::rieszMagnify() {
  int pBufferElements = static_cast<int>(processingBuffer->size());
  // Magnify only when processing buffer holds new images
  if (currentFrame >= pBufferElements)
    return;

  // Number of levels in pyramid
  levels = imgProcSettings->levels;

  Mat buffer_in, input, magnified, output;
  std::vector<cv::Mat> channels;
  int pChannels;
  static const double PI_PERCENT = M_PI / 100.0;

  // Process every frame in buffer that wasn't magnified yet
  while (currentFrame < pBufferElements) {
    // Grab oldest frame from processingBuffer and delete it to save memory
    buffer_in = processingBuffer->front().clone();
    if (currentFrame > 0) {
      processingBuffer->erase(processingBuffer->begin());
    }

    // Convert input image to 32bit float
    pChannels = buffer_in.channels();
    if (pChannels > 1) {
      // Convert color images to Lab
      if (!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
        // Convert color images to Lab
        buffer_in.convertTo(buffer_in, CV_32FC3, 1.0 / 255.0);
        cvtColor(buffer_in, buffer_in, COLOR_BGR2Lab);
        cv::split(buffer_in, channels);
        input = channels[0];
      } else {
        buffer_in.convertTo(input, CV_32FC1, 1.0 / 255.0);
      }

      // If first frame ever, init pointer and init class
      if (currentFrame == 0 || isnan(loCutoff->itsA[0]) ||
          isnan(hiCutoff->itsA[0])) {
        curPyr.reset();
        oldPyr.reset();
        loCutoff.reset();
        hiCutoff.reset();
        // Pyramids
        curPyr = std::make_shared<RieszPyramid>();
        oldPyr = std::make_shared<RieszPyramid>();
        curPyr->init(input, levels);
        oldPyr->init(input, levels);
        // Temporal Bandpass Filters, low and highpass (Butterworth)
        loCutoff = std::make_shared<RieszTemporalFilter>(
            imgProcSettings->coLow, imgProcSettings->framerate,
            curPyr->getSizes());
        hiCutoff = std::make_shared<RieszTemporalFilter>(
            imgProcSettings->coHigh, imgProcSettings->framerate,
            curPyr->getSizes());
        loCutoff->computeCoefficients();
        hiCutoff->computeCoefficients();

        // Make sure a valid frame is returned
        input.copyTo(output);
      } else {
        // Check if temporal filter setting was updated
        // Update low and highpass butterworth filter coefficients if changed in
        // GUI
        if (loCutoff->itsFrequency != imgProcSettings->coLow) {
          loCutoff->updateFrequency(imgProcSettings->coLow);
          loCutoff->resetMat();
          hiCutoff->resetMat();
          oldPyr->buildPyramid(input);
        }
        if (hiCutoff->itsFrequency != imgProcSettings->coHigh) {
          hiCutoff->updateFrequency(imgProcSettings->coHigh);
          hiCutoff->resetMat();
          loCutoff->resetMat();
          oldPyr->buildPyramid(input);
        }

        /* 1. BUILD RIESZ PYRAMID */
        curPyr->buildPyramid(input);

        /* 2. UNWRAPE PHASE TO GET HORIZ&VERTICAL / SIN&COS */
        curPyr->computePhaseDifferenceAndAmplitude(*oldPyr);
        // 3. BANDPASS FILTER ON EACH LEVEL
        for (int lvl = 0; lvl < curPyr->numLevels - 1; ++lvl) {
          loCutoff->IIRTemporalFilter(curPyr->pyrLevels[lvl].itsLowpassIIR,
                                      curPyr->pyrLevels[lvl].itsPhaseDiff, lvl);
          hiCutoff->IIRTemporalFilter(curPyr->pyrLevels[lvl].itsHighpassIIR,
                                      curPyr->pyrLevels[lvl].itsPhaseDiff, lvl);
        }

        // Shift current to prior for next iteration
        *oldPyr = *curPyr;

        // 4. AMPLIFY MOTION
        curPyr->amplify(imgProcSettings->amplification,
                        imgProcSettings->coWavelength * PI_PERCENT);

        /* 6. ADD MOTION TO ORIGINAL IMAGE */
        if (!currentFrame == 0) {
          magnified = curPyr->collapsePyramid();
        } else {
          magnified = input;
        }

        // Scale output image and convert back to 8bit unsigned
        if (pChannels > 1) {
          // Convert Lab image back to BGR
          magnified.convertTo(channels[0], CV_32FC1);
          cv::merge(channels, output);
          cvtColor(output, output, cv::COLOR_Lab2BGR);
          output.convertTo(output, CV_8UC3, 255.0, 1.0 / 255.0);
        } else {
          magnified.convertTo(output, CV_8UC1, 255.0, 1.0 / 255.0);
        }
      }
    }
    // Fill internal buffer with magnified image
    magnifiedBuffer.push_back(output);
    ++currentFrame;
  }
}

////////////////////////
/// Magnified Buffer ////
////////////////////////
Mat Magnificator::getFrameLast() {
  // Take newest image
  Mat img = this->magnifiedBuffer.back().clone();
  // Delete the oldest picture
  this->magnifiedBuffer.erase(magnifiedBuffer.begin());
  currentFrame = magnifiedBuffer.size();

  return img;
}

Mat Magnificator::getFrameFirst() {
  // Take oldest image
  Mat img = this->magnifiedBuffer.front().clone();
  // Delete the oldest picture
  this->magnifiedBuffer.erase(magnifiedBuffer.begin());
  currentFrame = magnifiedBuffer.size();

  return img;
}

Mat Magnificator::getFrameAt(int n) {
  int mLength = magnifiedBuffer.size();
  Mat img;

  if (n < mLength - 1)
    img = this->magnifiedBuffer.at(n).clone();
  else {
    img = getFrameLast();
  }
  // Delete the oldest picture
  currentFrame = magnifiedBuffer.size();

  return img;
}

bool Magnificator::hasFrame() { return !this->magnifiedBuffer.empty(); }

int Magnificator::getBufferSize() { return magnifiedBuffer.size(); }

void Magnificator::clearBuffer() {
  // Clear internal cache
  this->magnifiedBuffer.clear();
  this->lowpassHi.clear();
  this->lowpassLo.clear();
  this->motionPyramid.clear();
  this->downSampledMat = Mat();
  this->currentFrame = 0;
  oldPyr.reset();
  curPyr.reset();
  loCutoff.reset();
  hiCutoff.reset();
}

int Magnificator::getOptimalBufferSize(int fps) {
  // Calculate number of images needed to represent 2 seconds of film material
  unsigned int round = (unsigned int)std::max(2 * fps, 16);
  // Round to nearest higher power of 2
  round--;
  round |= round >> 1;
  round |= round >> 2;
  round |= round >> 4;
  round |= round >> 8;
  round |= round >> 16;
  round++;

  return round;
}

////////////////////////
/// Postprocessing //////
////////////////////////
void Magnificator::amplifyLaplacian(const Mat &src, Mat &dst,
                                    int currentLevel) {
  float currAlpha = (lambda / (delta * 8.0) - 1.0) * exaggeration_factor;
  // Set lowpassed&downsampled image and difference image with highest
  // resolution to 0, amplify every other level
  dst = (currentLevel == levels || currentLevel == 0)
            ? src * 0
            : src * std::min((float)imgProcSettings->amplification, currAlpha);
}

void Magnificator::attenuate(const Mat &src, Mat &dst) {
  // Attenuate only if image is not grayscale
  if (src.channels() > 2) {
    Mat planes[3];
    split(src, planes);
    planes[1] = planes[1] * imgProcSettings->chromAttenuation;
    planes[2] = planes[2] * imgProcSettings->chromAttenuation;
    merge(planes, 3, dst);
  }
}

void Magnificator::amplifyGaussian(const Mat &src, Mat &dst) {
  dst = src * imgProcSettings->amplification;
}
