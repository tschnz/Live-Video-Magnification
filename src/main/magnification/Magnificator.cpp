/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->Magnificator.cpp                                   */
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

#include "main/magnification/Magnificator.h"

////////////////////////
///Constructor /////////
////////////////////////
Magnificator::Magnificator(std::vector<Mat> *pBuffer,
                           ImageProcessingFlags *imageProcFlags,
                           ImageProcessingSettings *imageProcSettings) :
    processingBuffer(pBuffer),
    imgProcFlags(imageProcFlags),
    imgProcSettings(imageProcSettings),
    currentFrame(0)
{
    levels = 4;
    exaggeration_factor = 2.f;
    lambda = 0;
    delta = 0;
}

int Magnificator::calculateMaxLevels()
{
    Size s = processingBuffer->front().size();
    int highest = (s.width > s.height) ? s.height : s.width;
    int max = floor(log2(highest));

    return max;
}
int Magnificator::calculateMaxLevels(QRect r)
{
    Size s = Size(r.width(),r.height());
    int highest = (s.width > s.height) ? s.height : s.width;
    int max = floor(log2(highest));

    return max;
}
int Magnificator::calculateMaxLevels(Size s)
{
    int highest = (s.width > s.height) ? s.height : s.width;
    int max = floor(log2(highest));

    return max;
}

void Magnificator::BGR2YCbCr(const Mat &src, Mat &dst)
{
    Mat image = Mat::zeros(src.size(), src.type());

    const uchar* uc_pixel = src.data;
    uchar* c_pixel = image.data;
    for (int row = 0; row < src.rows; ++row)
    {
        uc_pixel = src.data + row*src.step;
        c_pixel = image.data + row*src.step;
        for (int col = 0; col < src.cols; ++col)
        {
            int R = uc_pixel[0];
            int G = uc_pixel[1];
            int B = uc_pixel[2];

            c_pixel[0] = 0 + 0.299*R + 0.587*G + 0.114*B;
            c_pixel[1] = 128 - 0.169*R - 0.331*G + 0.5*B;
            c_pixel[2] = 128 + 0.5*R - 0.419*G - 0.081*B;

            uc_pixel += 3;
            c_pixel += 3;
        }
   }

    dst = image;
}
void Magnificator::YCbCr2BGR(const Mat &src, Mat &dst)
{
    Mat image = Mat::zeros(src.size(), src.type());

    const uchar* uc_pixel = src.data;
    uchar* c_pixel = image.data;
    for (int row = 0; row < src.rows; ++row)
    {
        uc_pixel = src.data + row*src.step;
        c_pixel = image.data + row*src.step;
        for (int col = 0; col < src.cols; ++col)
        {
            int Y = uc_pixel[0];
            int Cb = uc_pixel[1];
            int Cr = uc_pixel[2];

            c_pixel[0] = Y + 1.4*(Cr-128);
            c_pixel[1] = Y - 0.343*(Cb-128) - 0.711*(Cr-128);
            c_pixel[2] = Y + 1.765*(Cb-128);

            uc_pixel += 3;
            c_pixel += 3;
        }
   }

    dst = image;
}

////////////////////////
///Magnification ///////
////////////////////////
void Magnificator:: colorMagnify() {
    int pBufferElements = processingBuffer->size();
    // Magnify only when processing buffer holds new images
    if(currentFrame >= pBufferElements)
        return;
    // Number of levels in pyramid
    //levels = DEFAULT_COL_MAG_LEVELS;
    levels = imgProcSettings->levels;
    Mat input, output, color, filteredFrame, downSampledFrame, filteredMat;
    std::vector<Mat> inputFrames, inputPyramid;

    int offset = 0;
    int pChannels;

    // Process every frame in buffer that wasn't magnified yet
    while(currentFrame < pBufferElements) {
        // Grab oldest frame from processingBuffer and delete it to save memory
        input = processingBuffer->front().clone();
        processingBuffer->erase(processingBuffer->begin());

        // Convert input image to 32bit float
        pChannels = input.channels();
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2))
            input.convertTo(input, CV_32FC1);
        else
            input.convertTo(input, CV_32FC3);

        // Save input frame to add motion later
        inputFrames.push_back(input);

        /* 1. SPATIAL FILTER, BUILD GAUSS PYRAMID */
        buildGaussPyrFromImg(input, levels, inputPyramid);

        /* 2. CONCAT EVERY SMALLEST FRAME FROM PYRAMID IN ONE LARGE MAT, 1COL = 1FRAME */
        downSampledFrame = inputPyramid.at(levels-1);
        img2tempMat(downSampledFrame, downSampledMat, getOptimalBufferSize(imgProcSettings->framerate));

        // Save how many frames we've currently downsampled
        ++currentFrame;
        ++offset;
    }

    /* 3. TEMPORAL FILTER */
    idealFilter(downSampledMat, filteredMat, imgProcSettings->coLow, imgProcSettings->coHigh, imgProcSettings->framerate);

    /* 4. AMPLIFY */
    amplifyGaussian(filteredMat, filteredMat);

    // Add amplified image (color) to every frame
    for (int i = currentFrame-offset; i < currentFrame; ++i) {

        /* 5. DE-CONCAT 1COL TO DOWNSAMPLED COLOR IMAGE */
        tempMat2img(filteredMat, i, downSampledFrame.size(), filteredFrame);

        /* 6. RECONSTRUCT COLOR IMAGE FROM PYRAMID */
        buildImgFromGaussPyr(filteredFrame, levels, color, input.size());

        /* 7. ADD COLOR IMAGE TO ORIGINAL IMAGE */
        output = inputFrames.front()+color;

        // Scale output image an convert back to 8bit unsigned
        double min,max;
        minMaxLoc(output, &min, &max);
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
            output.convertTo(output, CV_8UC1, 255.0/(max-min), -min * 255.0/(max-min));
        } else {
            output.convertTo(output, CV_8UC3, 255.0/(max-min), -min * 255.0/(max-min));
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
    if(currentFrame >= pBufferElements)
        return;
    // Number of levels in pyramid
//    levels = DEFAULT_LAP_MAG_LEVELS;
    levels = imgProcSettings->levels;

    Mat input, output, motion;
    vector<Mat> inputPyramid;
    int pChannels;

    // Process every frame in buffer that wasn't magnified yet
    while(currentFrame < pBufferElements) {
        // Grab oldest frame from processingBuffer and delete it to save memory
        input = processingBuffer->front().clone();
        if(currentFrame > 0)
            processingBuffer->erase(processingBuffer->begin());

        // Convert input image to 32bit float
        pChannels = input.channels();
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
            // Convert color images to YCrCb
            //BGR2YCbCr(input, input);
            input.convertTo(input, CV_32FC3, 1.0/255.0f);
            cvtColor(input, input, CV_BGR2YCrCb);
        }
        else
            input.convertTo(input, CV_32FC1, 1.0/255.0f);

        /* 1. SPATIAL FILTER, BUILD LAPLACE PYRAMID */
        buildLaplacePyrFromImg(input, levels, inputPyramid);

        // If first frame ever, save unfiltered pyramid
        if(currentFrame == 0) {
            lowpassHi = inputPyramid;
            lowpassLo = inputPyramid;
            motionPyramid = inputPyramid;
        } else {
            /* 2. TEMPORAL FILTER EVERY LEVEL OF LAPLACE PYRAMID */
            for (int curLevel = 0; curLevel < levels; ++curLevel) {
                iirFilter(inputPyramid.at(curLevel), motionPyramid.at(curLevel), lowpassHi.at(curLevel), lowpassLo.at(curLevel),
                          imgProcSettings->coLow, imgProcSettings->coHigh);
            }

            int w = input.size().width;
            int h = input.size().height;

            // Amplification variable
            delta = imgProcSettings->coWavelength / (8.0 * (1.0 + imgProcSettings->amplification));

            // Amplification Booster for better visualization
            exaggeration_factor = DEFAULT_LAP_MAG_EXAGGERATION;

            // compute representative wavelength, lambda
            // reduces for every pyramid level
            lambda = sqrt(w*w + h*h)/3.0;

            /* 3. AMPLIFY EVERY LEVEL OF LAPLACE PYRAMID */
            for (int curLevel = levels; curLevel >= 0; --curLevel) {
                amplifyLaplacian(motionPyramid.at(curLevel), motionPyramid.at(curLevel), curLevel);
                lambda /= 2.0;
            }
        }

        /* 4. RECONSTRUCT MOTION IMAGE FROM PYRAMID */
        buildImgFromLaplacePyr(motionPyramid, levels, motion);

        /* 5. ATTENUATE (if not grayscale) */
        attenuate(motion, motion);
        /* 6. ADD MOTION TO ORIGINAL IMAGE */
        if(currentFrame > 0)
            output = input+motion;
        else
            output = input;

        // Scale output image an convert back to 8bit unsigned
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
            // Convert YCrCb image back to BGR
            cvtColor(output, output, CV_YCrCb2BGR);
            output.convertTo(output, CV_8UC3, 255.0, 1.0/255.0);
            //YCbCr2BGR(output, output);
        }
        else
            output.convertTo(output, CV_8UC1, 255.0, 1.0/255.0);

        // Fill internal buffer with magnified image
        magnifiedBuffer.push_back(output);
        ++currentFrame;
    }
}

void Magnificator::waveletMagnify() {
    int pBufferElements = processingBuffer->size();
    // Magnify only when processing buffer holds new images
    if(currentFrame >= pBufferElements)
        return;
    // Number of levels in pyramid
//    this->levels = DEFAULT_DWT_MAG_LEVELS;
    levels = imgProcSettings->levels;

    Mat input, output, motion;
    vector< vector<Mat> > inputPyramid;
    int pChannels;

    // Process every frame in buffer that wasn't magnified yet
    while(currentFrame < pBufferElements) {
        // Grab oldest frame from processingBuffer and delete it to save memory
        input = processingBuffer->front().clone();

        if(currentFrame > 0)
            processingBuffer->erase(processingBuffer->begin());

        // Convert input image to 32bit float
        pChannels = input.channels();
        Mat* inputChannels = new Mat[pChannels];
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
            input.convertTo(input, CV_32FC3, 1.0/255.0f);
            // Convert color images to YCrCb
            cvtColor(input, input, CV_BGR2YCrCb);
        }
        else
            input.convertTo(input, CV_32FC1, 1.0/255.0f);

        // Split input image and only magnify L-channel
        split(input, inputChannels);

        /* !. SPATIAL FILTER, BUILD DWT */
        buildWaveletPyrFromImg(inputChannels[0].clone(), levels, inputPyramid);

        // If first frame ever, save unfiltered pyramid
        if(currentFrame == 0) {
            wlLowpassHi = inputPyramid;
            wlLowpassLo = inputPyramid;
            wlMotionPyramid = inputPyramid;
        } else {
            /* 2. TEMPORAL FILTER EVERY LEVEL OF LAPLACE PYRAMID */
            for (int curLevel = 0; curLevel < levels; ++curLevel) {
                iirWaveletFilter(inputPyramid.at(curLevel), wlMotionPyramid.at(curLevel), wlLowpassHi.at(curLevel), wlLowpassLo.at(curLevel),
                          imgProcSettings->coLow, imgProcSettings->coHigh);
            }

            int w = input.size().width;
            int h = input.size().height;

            // Amplification variable
            delta = imgProcSettings->coWavelength / (8.0 * (1.0 + imgProcSettings->amplification));

            // Amplification Booster for better visualization
            exaggeration_factor = DEFAULT_DWT_MAG_EXAGGERATION;

            // compute representative wavelength, lambda
            // reduces for every pyramid level
            lambda = sqrt(w*w + h*h)/3.0;

            /* 3. AMPLIFY EVERY LEVEL OF DWT */
            for (int curLevel = levels-1; curLevel >= 0; --curLevel) {
                amplifyWavelet(wlMotionPyramid.at(curLevel), wlMotionPyramid.at(curLevel), curLevel);
                lambda /= 2.0;
            }
        }

        /* 4. RECONSTRUCT MOTION IMAGE FROM PYRAMID */
        // While reconstructing
        buildImgFromWaveletPyr(wlMotionPyramid, motion, input.size());
        // Blur get softer motions
        GaussianBlur(motion, motion, Size(0,0), 1.25);

        // Merge array of motions back into one image
        for(int chn = 0; chn < pChannels; ++chn) {
            inputChannels[chn] = motion;
        }
        merge(inputChannels, pChannels, motion);

        /* 5. ATTENUATE (if not grayscale) */
        attenuate(motion, motion);

        /* 6. ADD MOTION TO ORIGINAL IMAGE */
        if(currentFrame > 0)
            output = input+motion;
        else
            output = input;

        // Scale output image an convert back to 8bit unsigned
        if(!(imgProcFlags->grayscaleOn || pChannels <= 2)) {
            // Convert YCrCb image back to BGR
            cvtColor(output, output, CV_YCrCb2BGR);
            output.convertTo(output, CV_8UC3, 255.0, 1.0/255.0);
        }
        else
            output.convertTo(output, CV_8UC1, 255.0, 1.0/255.0);

        // Fill internal buffer with magnified image
        magnifiedBuffer.push_back(output);
        ++currentFrame;

        delete [] inputChannels;
    }
}
////////////////////////
///Magnified Buffer ////
////////////////////////
Mat Magnificator::getFrameLast()
{
    // Take newest image
    Mat img = this->magnifiedBuffer.back().clone();
    // Delete the oldest picture
    this->magnifiedBuffer.erase(magnifiedBuffer.begin());
    currentFrame = magnifiedBuffer.size();

    return img;
}

Mat Magnificator::getFrameFirst()
{
    // Take oldest image
    Mat img = this->magnifiedBuffer.front().clone();
    // Delete the oldest picture
    this->magnifiedBuffer.erase(magnifiedBuffer.begin());
    currentFrame = magnifiedBuffer.size();

    return img;
}

Mat Magnificator::getFrameAt(int n)
{
    int mLength = magnifiedBuffer.size();
    Mat img;

    if(n < mLength-1)
        img = this->magnifiedBuffer.at(n).clone();
    else {
        img = getFrameLast();
    }
    // Delete the oldest picture
    currentFrame = magnifiedBuffer.size();

    return img;
}

int Magnificator::getBufferSize()
{
    return magnifiedBuffer.size();
}

void Magnificator::clearBuffer()
{
    // Clear internal cache
    this->magnifiedBuffer.clear();
    this->lowpassHi.clear();
    this->lowpassLo.clear();
    this->motionPyramid.clear();
    this->wlLowpassHi.clear();
    this->wlLowpassLo.clear();
    this->wlMotionPyramid.clear();
    this->downSampledMat = Mat();
    this->currentFrame = 0;
}



int Magnificator::getOptimalBufferSize(int fps)
{
    // Calculate number of images needed to represent 2 seconds of film material
    unsigned int round = (unsigned int) std::max(2*fps,16);
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
///Postprocessing //////
////////////////////////
void Magnificator::amplifyLaplacian(const Mat &src, Mat &dst, int currentLevel)
{
    float currAlpha = (lambda/(delta*8.0) - 1.0) * exaggeration_factor;
    // Set lowpassed&downsampled image and difference image with highest resolution to 0,
    // amplify every other level
    dst = (currentLevel == levels || currentLevel == 0) ? src * 0
                                                        : src * std::min((float)imgProcSettings->amplification, currAlpha);
}

void Magnificator::attenuate(const Mat &src, Mat &dst)
{
    // Attenuate only if image is not grayscale
    if(src.channels() > 2)
    {
        Mat planes[3];
        split(src, planes);
        planes[1] = planes[1] * imgProcSettings->chromAttenuation;
        planes[2] = planes[2] * imgProcSettings->chromAttenuation;
        merge(planes, 3, dst);
    }
}

void Magnificator::amplifyWavelet(const vector<Mat> &src, vector<Mat> &dst, int currentLevel)
{
    float currAlpha = (lambda/(delta*8.0) - 1.0) * exaggeration_factor;
    // Set reference image (lowest resolution) to 0, amplify every other channel
    for(int dims = 0; dims < 3; ++dims) {
        dst.at(dims) = src.at(dims) * std::min((float)imgProcSettings->amplification, currAlpha);
    }
    if(currentLevel == levels-1)
        dst.at(3) = src.at(3) * 0;
}

void Magnificator::amplifyGaussian(const Mat &src, Mat &dst)
{
    dst = src * imgProcSettings->amplification;
}
