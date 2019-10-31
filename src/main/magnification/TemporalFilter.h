/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->TemporalFilter.h                                   */
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

#ifndef TEMPORALFILTER_H
#define TEMPORALFILTER_H

// Project
#include "main/helper/ComplexMat.h"
// OpenCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#define M_PI 3.14159265358979323846264338327950288

using namespace cv;
using namespace std;

////////////////////////
///Helper //////////////
////////////////////////
/*!
 * \brief img2tempMat (Color Magnification) Takes a Mat, reshapes it in 1 column with width*height rows & concatenates it
 *  on the right side of dst.
 * \param frame Input frame.
 * \param dst Matrice of concatenated reshaped frames, 1 column = 1 frame. Most right frame is most actual frame.
 * \param maxImages Maximum number of images hold in dst. If columns of dst > maxImages, the most
 *  left column is deleted. Value should be a power of 2 for fast DFT.
 */
void img2tempMat(const Mat &frame, Mat &dst, int maxImages);
/*!
 * \brief tempMat2img (Color Magnification) Takes a Mat of line-concatenated frames and reshapes 1 column back into a frame.
 * \param src Mat of concatenated frames.
 * \param position The column in src that shall get reshaped.
 * \param frameSize The destination size the reshaped frame shall have.
 * \param frame Output frame with size frameSize.
 */
void tempMat2img(const Mat &src, int position, const Size &frameSize, Mat &frame);
/*!
 * \brief createIdealBandpassFilter (Color Magnification) Creates a filter mask for an ideal filter.
 * \param filter Filter mask.
 * \param cutoffLo Lower cutoff frequency.
 * \param cutoffHi Upper cutoff frequency.
 * \param framerate Framerate of processed video.
 */
void createIdealBandpassFilter(Mat &filter, double cutoffLo, double cutoffHi, double framerate);

////////////////////////
///Filter //////////////
////////////////////////
/*!
 * \brief iirFilter (Euler Magnification) Applies an iirFilter (in space domain) on 1 level of a Laplace Pyramid.
 * \param src Newest input image of a level of a Laplace Pyramid.
 * \param dst Iir filtered level of a Laplace Pyramid.
 * \param lowpassHi Holding the informations about the previous (high) lowpass filtered images of a level.
 * \param lowpassLo Holding the informations about the previous (low) lowpass filtered images of a level.
 * \param cutoffLo Lower cutoff frequency.
 * \param cutoffHi Higher cutoff frequency.
 */
void iirFilter(const Mat &src, Mat &dst, Mat &lowpassHi, Mat &lowpassLo, double cutoffLo, double cutoffHi);
/*!
 * \brief iirWaveletFilter (Wavelet Magnification) Applies an iirFilter (in space domain) on 1 level of a DWT.
 * \param src
 * \param dst
 * \param lowpassHi
 * \param lowpassLo
 * \param cutoffLo
 * \param cutoffHi
 */
void iirWaveletFilter(const vector<Mat> &src, vector<Mat> &dst, vector<Mat> &lowpassHi, vector<Mat> &lowpassLo, double cutoffLo, double cutoffHi);
/*!
 * \brief idealFilter (Color Magnification)
 * \param src
 * \param dst
 * \param cutoffLo
 * \param cutoffHi
 * \param framerate
 */
void idealFilter(const Mat &src, Mat &dst, double cutoffLo, double cutoffHi, double framerate);

///
// From https://github.com/tbl3rd/Pyramids
///
// Temp Filter for Riesz Pyramid
class RieszTemporalFilter {

    RieszTemporalFilter &operator=(const RieszTemporalFilter &);
    RieszTemporalFilter(const RieszTemporalFilter &);

public:
    RieszTemporalFilter(): itsFrequency(0.0), itsFramerate(0.0), itsA(), itsB() { }
    RieszTemporalFilter(double frq, double fps): itsFrequency(frq), itsFramerate(fps), itsA(), itsB() { }

    double itsFrequency;
    double itsFramerate;
    std::vector<double> itsA;
    std::vector<double> itsB;

    // Compute this filter's Butterworth coefficients for the sampling
    // frequency, fps (frames per second).
    //
    void updateFramerate(double framerate);
    void updateFrequency(double f);
    void computeCoefficients();

    void passEach(cv::Mat &result,
                  const cv::Mat &phase,
                  const cv::Mat &prior);

    void pass(CompExpMat &result,
              const CompExpMat &phase,
              const CompExpMat &prior);
};

#endif // TEMPORALFILTER_H
