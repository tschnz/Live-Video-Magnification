#pragma once
// Project
#include "../helper/ComplexMat.h"
// OpenCV
#include <opencv2/core.hpp>
// C++
#include <algorithm>
#include <complex>
#include <vector>

////////////////////////
/// Helper //////////////
////////////////////////
/*!
 * \brief img2tempMat (Color Magnification) Takes a Mat, reshapes it in 1 column
 * with width*height rows & concatenates it on the right side of dst. \param
 * frame Input frame. \param dst Matrice of concatenated reshaped frames, 1
 * column = 1 frame. Most right frame is most actual frame. \param maxImages
 * Maximum number of images hold in dst. If columns of dst > maxImages, the most
 *  left column is deleted. Value should be a power of 2 for fast DFT.
 */
void img2tempMat(const cv::Mat &frame, cv::Mat &dst, int maxImages);
/*!
 * \brief tempMat2img (Color Magnification) Takes a Mat of line-concatenated
 * frames and reshapes 1 column back into a frame. \param src Mat of
 * concatenated frames. \param position The column in src that shall get
 * reshaped. \param frameSize The destination size the reshaped frame shall
 * have. \param frame Output frame with size frameSize.
 */
void tempMat2img(const cv::Mat &src, int position, const cv::Size &frameSize,
                 cv::Mat &frame);
/*!
 * \brief createIdealBandpassFilter (Color Magnification) Creates a filter mask
 * for an ideal filter. \param filter Filter mask. \param cutoffLo Lower cutoff
 * frequency. \param cutoffHi Upper cutoff frequency. \param framerate Framerate
 * of processed video.
 */
void createIdealBandpassFilter(cv::Mat &filter, double cutoffLo,
                               double cutoffHi, double framerate);

////////////////////////
/// Filter //////////////
////////////////////////
/*!
 * \brief iirFilter (Euler Magnification) Applies an iirFilter (in space domain)
 * on 1 level of a Laplace Pyramid. \param src Newest input image of a level of
 * a Laplace Pyramid. \param dst Iir filtered level of a Laplace Pyramid. \param
 * lowpassHi Holding the informations about the previous (high) lowpass filtered
 * images of a level. \param lowpassLo Holding the informations about the
 * previous (low) lowpass filtered images of a level. \param cutoffLo Lower
 * cutoff frequency. \param cutoffHi Higher cutoff frequency.
 */
void iirFilter(const cv::Mat &src, cv::Mat &dst, cv::Mat &lowpassHi,
               cv::Mat &lowpassLo, double cutoffLo, double cutoffHi);
/*!
 * \brief idealFilter (Color Magnification)
 * \param src
 * \param dst
 * \param cutoffLo
 * \param cutoffHi
 * \param framerate
 */
void idealFilter(const cv::Mat &src, cv::Mat &dst, double cutoffLo,
                 double cutoffHi, double framerate);

// Temp Filter for Riesz Pyramid
class RieszTemporalFilter {

  RieszTemporalFilter &operator=(const RieszTemporalFilter &);
  RieszTemporalFilter(const RieszTemporalFilter &);

public:
  RieszTemporalFilter(double frq, double fps,
                      std::vector<std::pair<int, int>> lvlSizes);

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

  void passEach(cv::Mat &result, const cv::Mat &phase, const cv::Mat &prior);

  void pass(CompExpMat &result, const CompExpMat &phase,
            const CompExpMat &prior);

  void IIRTemporalFilter(CompExpMat &result, const CompExpMat &phaseDiff,
                         int lvl);

  void resetMat();

private:
  size_t numPyrLvls;
  std::vector<CompExpMat> itsRegister0;
  std::vector<CompExpMat> itsRegister1;
  std::vector<CompExpMat> itsPhase;
};