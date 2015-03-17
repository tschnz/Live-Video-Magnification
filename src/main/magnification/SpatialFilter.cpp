/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->SpatialFilter.cpp                                  */
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

#include "main/magnification/SpatialFilter.h"

////////////////////////
/// Downsampling ///////
////////////////////////
void buildGaussPyrFromImg(const Mat &img, const int levels, vector<Mat> &pyr)
{
    pyr.clear();
    Mat currentLevel = img;

    for (int level = 0; level < levels; ++level) {
        Mat down;
        pyrDown(currentLevel, down);
        pyr.push_back(down);
        currentLevel = down;
    }
}


void buildLaplacePyrFromImg(const Mat &img, const int levels, vector<Mat> &pyr)
{
    pyr.clear();
    Mat currentLevel = img;

    for (int level = 0; level < levels; ++level) {
        Mat down,up;
        pyrDown(currentLevel, down);
        pyrUp(down, up, currentLevel.size());
        Mat laplace = currentLevel - up;
        pyr.push_back(laplace);
        currentLevel = down;
    }
    pyr.push_back(currentLevel);
}

void buildWaveletPyrFromImg(const Mat &img, const int levels, vector<vector<Mat> > &pyr, int SHRINK_TYPE, float SHRINK_T)
{
    float c,dh,dv,dd;
    vector<Mat> levelVector;
    int width = img.cols;
    int height = img.rows;
    Mat curFrame = img.clone();
    pyr = vector< vector<Mat> >(levels);


    for (int lvl=0;lvl<levels;lvl++)
    {
        //Adjust size for this iteration
        height = height/2;
        width = width/2;

        // Delete downsampled image from last iteration
        if(lvl > 0) {
            levelVector.clear();
            pyr[lvl-1].pop_back();
        }
        // Create new Mats to write DWT into
        for(int dir = 0; dir < 4; dir++) {
            levelVector.push_back(Mat::zeros(height,width,CV_32F));
        }

        for (int y=0;y<height;y++)
        {
            for (int x=0; x<width;x++)
            {
                // DWT
                c=(curFrame.at<float>(2*y,2*x)+curFrame.at<float>(2*y,2*x+1)+curFrame.at<float>(2*y+1,2*x)+curFrame.at<float>(2*y+1,2*x+1))*0.5;
                dh=(curFrame.at<float>(2*y,2*x)+curFrame.at<float>(2*y+1,2*x)-curFrame.at<float>(2*y,2*x+1)-curFrame.at<float>(2*y+1,2*x+1))*0.5;
                dv=(curFrame.at<float>(2*y,2*x)+curFrame.at<float>(2*y,2*x+1)-curFrame.at<float>(2*y+1,2*x)-curFrame.at<float>(2*y+1,2*x+1))*0.5;
                dd=(curFrame.at<float>(2*y,2*x)-curFrame.at<float>(2*y,2*x+1)-curFrame.at<float>(2*y+1,2*x)+curFrame.at<float>(2*y+1,2*x+1))*0.5;

                // Shrinkage
                switch(SHRINK_TYPE)
                {
                case HARD:
                    dh=wl_hard_shrink(dh,SHRINK_T);
                    dv=wl_hard_shrink(dv,SHRINK_T);
                    dd=wl_hard_shrink(dd,SHRINK_T);
                    break;
                case SOFT:
                    dh=wl_soft_shrink(dh,SHRINK_T);
                    dv=wl_soft_shrink(dv,SHRINK_T);
                    dd=wl_soft_shrink(dd,SHRINK_T);
                    break;
                case GARROT:
                    dh=wl_garrot_shrink(dh,SHRINK_T);
                    dv=wl_garrot_shrink(dv,SHRINK_T);
                    dd=wl_garrot_shrink(dd,SHRINK_T);
                    break;
                }

                // Save downsampled image on last position in vector
                levelVector[3].at<float>(y,x)=c;
                // Save dHorizontal, dVertical and dDiagonal
                levelVector[0].at<float>(y,x)=dh;
                levelVector[1].at<float>(y,x)=dv;
                levelVector[2].at<float>(y,x)=dd;
                }
        }
        // Update current image that is going to be downsampled
        levelVector[3].copyTo(curFrame);
        // Save current Vector in output pyramid
        pyr[lvl] = levelVector;
    }
}

////////////////////////
/// Upsampling /////////
////////////////////////
void buildImgFromGaussPyr(const Mat &pyr, const int levels, Mat &dst, Size size)
{
    Mat currentLevel = pyr.clone();

    for (int level = 0; level < levels; ++level) {
        Mat up;
        pyrUp(currentLevel, up);
        currentLevel = up;
    }
    // Resize the image to comprehend errors due to rounding
    resize(currentLevel,currentLevel,size);
    currentLevel.copyTo(dst);
}

void buildImgFromLaplacePyr(const vector<Mat> &pyr, const int levels, Mat &dst)
{
    Mat currentLevel = pyr[levels];

    for (int level = levels-1; level >= 0; --level) {
        Mat up;
        pyrUp(currentLevel, up, pyr[level].size());
        currentLevel = up+pyr[level];
    }
    dst = currentLevel.clone();
}

void buildImgFromWaveletPyr(const vector<vector<Mat> > &pyr, Mat &dst, Size origSize, int SHRINK_TYPE, float SHRINK_T)
{
    int levels = pyr.size();
    float c,dh,dv,dd;
    Mat tmp;
    int width,height;

    // First picture that will be upsampled is beeing hold in pyramid
    Mat currentRecon = pyr[levels-1][3];
    currentRecon.convertTo(currentRecon,CV_32FC1);

    // For every level, beginning from the last one
    for (int lvl=levels-1;lvl>=0;lvl--)
    {
        // Adjust size to next level
        Size size = (lvl == 0) ? origSize : pyr[lvl-1][0].size();
        height = size.height;
        width = size.width;

        // Calculate image for next level
        tmp = Mat::zeros(height, width, CV_32FC1);
        for (int y=0;y<height/2;y++)
        {
            for (int x=0; x<width/2;x++)
            {
                c=currentRecon.at<float>(y,x);
                dh=pyr[lvl][0].at<float>(y,x);
                dv=pyr[lvl][1].at<float>(y,x);
                dd=pyr[lvl][2].at<float>(y,x);

                // Shrinkage
                switch(SHRINK_TYPE)
                {
                case HARD:
                    dh=wl_hard_shrink(dh,SHRINK_T);
                    dv=wl_hard_shrink(dv,SHRINK_T);
                    dd=wl_hard_shrink(dd,SHRINK_T);
                    break;
                case SOFT:
                    dh=wl_soft_shrink(dh,SHRINK_T);
                    dv=wl_soft_shrink(dv,SHRINK_T);
                    dd=wl_soft_shrink(dd,SHRINK_T);
                    break;
                case GARROT:
                    dh=wl_garrot_shrink(dh,SHRINK_T);
                    dv=wl_garrot_shrink(dv,SHRINK_T);
                    dd=wl_garrot_shrink(dd,SHRINK_T);
                    break;
                }

                // iDWT
                tmp.at<float>(y*2,x*2)=0.5*(c+dh+dv+dd);
                tmp.at<float>(y*2,x*2+1)=0.5*(c-dh+dv-dd);
                tmp.at<float>(y*2+1,x*2)=0.5*(c+dh-dv-dd);
                tmp.at<float>(y*2+1,x*2+1)=0.5*(c-dh-dv+dd);
            }
        }
        // Next picture that will be upsampled
        tmp.copyTo(currentRecon);
    }
    // Final output
    dst = currentRecon.clone();
}

////////////////////////
/// Helper /////////////
////////////////////////
float wl_hard_shrink(float d,float T)
{
    float res;
    if(fabs(d)>T)
    {
        res=d;
    }
    else
    {
        res=0;
    }

    return res;
}

float wl_sgn(float x)
{
    float res=0;
    if(x==0)
    {
        res=0;
    }
    if(x>0)
    {
        res=1;
    }
    if(x<0)
    {
        res=-1;
    }
    return res;
}

float wl_soft_shrink(float d,float T)
{
    float res;
    if(fabs(d)>T)
    {
        res=wl_sgn(d)*(fabs(d)-T);
    }
    else
    {
        res=0;
    }

    return res;
}


float wl_garrot_shrink(float d,float T)
{
    float res;
    if(fabs(d)>T)
    {
        res=d-((T*T)/d);
    }
    else
    {
        res=0;
    }

    return res;
}
