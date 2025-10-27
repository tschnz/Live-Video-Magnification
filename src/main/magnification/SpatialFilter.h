#pragma once

// OpenCV
#include <opencv2/opencv.hpp>
// C++
#include <math.h>
#include <vector>

using namespace std;
using namespace cv;

// Wavelet Filtertypes
#define NONE 0   // no filter
#define HARD 1   // hard shrinkage
#define SOFT 2   // soft shrinkage
#define GARROT 3 // garrot filter

////////////////////////
/// Downsampling ///////
////////////////////////
/*!
 * \brief buildGaussPyrFromImg Builds a Gauss Pyramid from given image and saves
 * every level of the pyramid in a vector. \param img Source image. \param
 * levels Number of times the image is downsampled. \param pyr Vector that holds
 * every level of the pyramid. Last element is smallest image.
 */
void buildGaussPyrFromImg(const Mat &img, const int levels, vector<Mat> &pyr);
/*!
 * \brief buildLaplacePyrFromImg Builds a Laplace Pyramid from given image and
 * saves every level of the pyramid in a vector (remember: a LaplacePyr saves
 * the difference of a resolution downsampled image p-1 and its origin p, both
 * having the size of p). \param img Source image. \param levels Number of times
 * the image is downsampled. \param pyr Vector that holds every level of the
 * pyramid. Last element is smallest image (not the difference).
 */
void buildLaplacePyrFromImg(const Mat &img, const int levels, vector<Mat> &pyr);
/*!
 * \brief buildWaveletPyrFromImg Computes the discrete wavelet transform (DWT)
 * with a Haar Wavelet as base. \param img Source image. \param levels Numbers
 * of transformations that is done. \param pyr The DWT Pyramid, holding the
 * different levels on the 1st dimension, and for each three images of the
 * details/coeeficients on the 2nd dimension \param SHRINK_TYPE Noise reduction
 * type. \param SHRINK_T Noise reduction value.
 */
void buildWaveletPyrFromImg(const Mat &img, const int levels,
                            vector<vector<Mat>> &pyr, int SHRINK_TYPE = 0,
                            float SHRINK_T = 10.f);

////////////////////////
/// Upsampling /////////
////////////////////////
/*!
 * \brief buildImgFromGaussPyr Reconstructs an image from a given Gauss Pyramid.
 * \param pyr Vector that holds the image levels of the Pyramid.
 * \param levels Number of levels that are used to reconstruct the image. Should
 * be < pyr.size. \param dst Destination Mat for upsampled image. \param size
 * Destination size of upsampled image.
 */
void buildImgFromGaussPyr(const Mat &pyr, const int levels, Mat &dst,
                          Size size);
/*!
 * \brief buildImgFromLaplacePyr Reconstructs an image from a given Laplace
 * Pyramid. \param pyr Vector that holds the image levels of the Pyramid. \param
 * levels Number of levels that are used to reconstruct the image. Should be <
 * pyr.size. \param dst Destination Mat for upsampled image.
 */
void buildImgFromLaplacePyr(const vector<Mat> &pyr, const int levels, Mat &dst);
/*!
 * \brief buildImgFromWaveletPyr Reconstructs an image from a DWT.
 * \param pyr The pyramid, holding the levels on the 1st dimension and
 * coefficients on the 2nd dimension. \param dst Destination Mat for upsampled
 * image. \param origSize Destination size of upsampled image. \param
 * SHRINK_TYPE Noise reduction type. \param SHRINK_T Noise reduction value.
 */
void buildImgFromWaveletPyr(const vector<vector<Mat>> &pyr, Mat &dst,
                            Size origSize, int SHRINK_TYPE = 0,
                            float SHRINK_T = 10.f);

////////////////////////
/// Helper /////////////
////////////////////////
/*!
 * \brief wl_hard_shrink Noise reduction method for DWT.
 * \param d Value of coefficient.
 * \param T Threshold.
 * \return
 */
float wl_hard_shrink(float d, float T);
/*!
 * \brief wl_sgn Helper for Soft Shring noise reduction. Signs a Bit with
 * value={-1,0,1} \param x Bit to sign. \return
 */
float wl_sgn(float x);
/*!
 * \brief wl_soft_shrink Noise reduction method for DWT.
 * \param d Value of coefficient.
 * \param T Threshold.
 * \return
 */
float wl_soft_shrink(float d, float T);
/*!
 * \brief wl_garrot_shrink Noise reduction method for DWT.
 * \param d Value of coefficient.
 * \param T Threshold.
 * \return
 */
float wl_garrot_shrink(float d, float T);