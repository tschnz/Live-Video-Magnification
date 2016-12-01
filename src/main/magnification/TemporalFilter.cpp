/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->TemporalFilter.cpp                                 */
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

#include "main/magnification/TemporalFilter.h"

////////////////////////
///Filter //////////////
////////////////////////
void iirFilter(const Mat &src, Mat &dst, Mat &lowpassHi, Mat &lowpassLo,
               double cutoffLo, double cutoffHi)
{
    // Set minimum for cutoff, so low cutoff gets faded out
    if(cutoffLo == 0)
        cutoffLo = 0.01;

    /* The higher cutoff*, the faster the lowpass* image of the lowpass* pyramid gets faded out.
     * That means, a high cutoff weights new images (= \param src)
     * more than the old ones (= \param lowpass*), so long lasting movements are faded out fast.
     * The other way, a low cutoff evens out fast movements ocurring only in a few number of src images. */

    Mat tmp1 = (1-cutoffHi)*lowpassHi + cutoffHi*src;
    Mat tmp2 = (1-cutoffLo)*lowpassLo + cutoffLo*src;
    lowpassHi = tmp1;
    lowpassLo = tmp2;

    dst = lowpassHi - lowpassLo;
}

void iirWaveletFilter(const vector<Mat> &src, vector<Mat> &dst, vector<Mat> &lowpassHi, vector<Mat> &lowpassLo,
                      double cutoffLo, double cutoffHi)
{
    // Set minimum for cutoff, so low cutoff gets faded out
    if(cutoffLo == 0)
        cutoffLo = 0.01;

    /* The higher cutoff*, the faster the lowpass* image of the lowpass* pyramid gets faded out.
     * That means, a high cutoff weights new images (= \param src)
     * more than the old ones (= \param lowpass*), so long lasting movements are faded out fast.
     * The other way, a low cutoff evens out fast movements ocurring only in a few number of src images. */

    // Do this for every detail/coefficient image
    for(int dims = 0; dims < 3; dims++) {
        Mat tmp1 = (1-cutoffHi)*lowpassHi[dims] + cutoffHi*src[dims];
        Mat tmp2 = (1-cutoffLo)*lowpassLo[dims] + cutoffLo*src[dims];
        lowpassHi[dims] = tmp1;
        lowpassLo[dims] = tmp2;

        dst[dims] = lowpassHi[dims] - lowpassLo[dims];
    }
}

void idealFilter(const Mat &src, Mat &dst , double cutoffLo, double cutoffHi, double framerate)
{
    if(cutoffLo == 0.00)
        cutoffLo += 0.01;

    int channelNrs = src.channels();
    Mat *channels = new Mat[channelNrs];
    split(src, channels);

    // Apply filter on each channel individually
    for (int curChannel = 0; curChannel < channelNrs; ++curChannel) {
        Mat current = channels[curChannel];
        Mat tempImg;

        int width = current.cols;
        int height = getOptimalDFTSize(current.rows);

        copyMakeBorder(current, tempImg,
                       0, height - current.rows,
                       0, width - current.cols,
                       BORDER_CONSTANT, Scalar::all(0));

        // DFT
        dft(tempImg, tempImg, DFT_ROWS | DFT_SCALE);

        // construct Filter
        Mat filter = tempImg.clone();
        createIdealBandpassFilter(filter, cutoffLo, cutoffHi, framerate);

        // apply
        mulSpectrums(tempImg, filter, tempImg, DFT_ROWS);

        // inverse
        idft(tempImg, tempImg, DFT_ROWS | DFT_SCALE);

        tempImg(Rect(0, 0, current.cols, current.rows)).copyTo(channels[curChannel]);
    }
    merge(channels, channelNrs, dst);

    normalize(dst, dst, 0, 1, CV_MINMAX);
    delete [] channels;
}

void createIdealBandpassFilter(Mat &filter, double cutoffLo, double cutoffHi, double framerate)
{
    float width = filter.cols;
    float height = filter.rows;

    // Calculate frequencies according to framerate and size
    double fl = 2 * cutoffLo * width / framerate;
    double fh = 2 * cutoffHi * width / framerate;

    double response;

    // Create the filtermask, looks like the quarter of a circle
    for(int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if(x >= fl && x <= fh)
                response = 1.0f;
            else
                response = 0.0f;

            filter.at<float>(y,x) = response;
        }
    }
}

////////////////////////
///Helper //////////////
////////////////////////
void img2tempMat(const Mat &frame, Mat &dst, int maxImages)
{
    // Reshape in 1 column
    Mat reshaped = frame.reshape(frame.channels(), frame.cols*frame.rows).clone();

    if(frame.channels() == 1)
        reshaped.convertTo(reshaped, CV_32FC1);
    else
        reshaped.convertTo(reshaped, CV_32FC3);

    // First frame
    if(dst.cols == 0) {
        reshaped.copyTo(dst);
    }
    // Later frames
    else {
        hconcat(dst,reshaped,dst);
    }

    // If dst reaches maximum, delete the first column (eg the oldest image)
    if(dst.cols > maxImages && maxImages != 0) {
        dst.colRange(1,dst.cols).copyTo(dst);
    }
}

void tempMat2img(const Mat &src, int position, const Size &frameSize, Mat &frame)
{
    Mat line = src.col(position).clone();
    frame = line.reshape(line.channels(), frameSize.height).clone();
}
