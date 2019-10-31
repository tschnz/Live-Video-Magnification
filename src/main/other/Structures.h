/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->Structures.h                                       */
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

#ifndef STRUCTURES_H
#define STRUCTURES_H

// Qt
#include <QtCore/QRect>

struct ImageProcessingSettings{
    double amplification;
    double coWavelength;
    double coLow;
    double coHigh;
    double chromAttenuation;
    int frameWidth;
    int frameHeight;
    double framerate;
    int levels;

    ImageProcessingSettings() :
        amplification(0.0),
        coWavelength(0.0),
        coLow(0.1),
        coHigh(0.4),
        chromAttenuation(0.0),
        frameWidth(0),
        frameHeight(0),
        framerate(0.0),
        levels(4)
    {
    }
};

struct ImageProcessingFlags{
    bool grayscaleOn;
    bool colorMagnifyOn;
    bool laplaceMagnifyOn;
    bool rieszMagnifyOn;

    ImageProcessingFlags() :
        grayscaleOn(false),
        colorMagnifyOn(false),
        laplaceMagnifyOn(false),
        rieszMagnifyOn(false)
    {
    }
};

struct MouseData{
    QRect selectionBox;
    bool leftButtonRelease;
    bool rightButtonRelease;
};

struct ThreadStatisticsData{
    int averageFPS;
    double nFramesProcessed;
    double averageVidProcessingFPS;
};

#endif // STRUCTURES_H
