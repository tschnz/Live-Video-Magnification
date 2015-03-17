/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->SharedImageBuffer.cpp                              */
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

#include "main/helper/SharedImageBuffer.h"

SharedImageBuffer::SharedImageBuffer()
{
    // Initialize variables(s)
    nArrived=0;
}

void SharedImageBuffer::add(int deviceNumber, Buffer<Mat>* imageBuffer)
{
    // Add image buffer to map
    imageBufferMap[deviceNumber]=imageBuffer;
}

Buffer<Mat>* SharedImageBuffer::getByDeviceNumber(int deviceNumber)
{
    return imageBufferMap[deviceNumber];
}

void SharedImageBuffer::removeByDeviceNumber(int deviceNumber)
{
    // Remove buffer for device from imageBufferMap
    imageBufferMap.remove(deviceNumber);
}

void SharedImageBuffer::wakeAll()
{
    QMutexLocker locker(&mutex);
    wc.wakeAll();
}

bool SharedImageBuffer::containsImageBufferForDeviceNumber(int deviceNumber)
{
    return imageBufferMap.contains(deviceNumber);
}
