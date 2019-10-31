/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->Config.h 		                                    */
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

#ifndef CONFIG_H
#define CONFIG_H

// FPS statistics queue lengths
#define PROCESSING_FPS_STAT_QUEUE_LENGTH    32
#define CAPTURE_FPS_STAT_QUEUE_LENGTH       32

// Image buffer size
#define DEFAULT_IMAGE_BUFFER_SIZE           1
// Drop frame if image/frame buffer is full
#define DEFAULT_DROP_FRAMES                 false
// Thread priorities
#define DEFAULT_CAP_THREAD_PRIO             QThread::NormalPriority
#define DEFAULT_PROC_THREAD_PRIO            QThread::HighPriority
#define DEFAULT_PLAY_THREAD_PRIO            QThread::NormalPriority

// IMAGE PROCESSING
#define DEFAULT_COL_MAG_LEVELS              3

#define DEFAULT_LAP_MAG_EXAGGERATION        2.0
#define DEFAULT_LAP_MAG_LEVELS              4

// General Default on Startup
#define DEFAULT_GRAYSCALE                   false
#define DEFAULT_MAGNIFY_TYPE                0 // Options: [NONE=0,-1;COLOR=1;LAPLACE=2;RIESZ=3]
#define DEFAULT_AMPLIFICATION               0
#define DEFAULT_COWAVELENGTH                0
#define DEFAULT_COLOW                       0.0
#define DEFAULT_COHIGH                      0.0
#define DEFAULT_CHROMATTENUATION            0
// Default for Color Magnification
#define DEFAULT_CM_AMPLIFICATION            100
#define DEFAULT_CM_COWAVELENGTH             1000
#define DEFAULT_CM_COLOW                    0.84
#define DEFAULT_CM_COHIGH                   1.43
#define DEFAULT_CM_CHROMATTENUATION         0
// Default for Motion Magnification
#define DEFAULT_MM_AMPLIFICATION            20
#define DEFAULT_MM_COWAVELENGTH             50
#define DEFAULT_MM_COLOW                    20.0
#define DEFAULT_MM_COHIGH                   40.0
#define DEFAULT_MM_CHROMATTENUATION         0
// Default for Phase Based Riesz Magnification
#define DEFAULT_PB_AMPLIFICATION            25
#define DEFAULT_PB_COWAVELENGTH             25
#define DEFAULT_PB_COLOW                    0.1
#define DEFAULT_PB_COHIGH                   1.0

#endif // CONFIG_H
