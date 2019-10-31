/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->MagnifyOptions.h                                   */
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

#ifndef MAGNIFYOPTIONS_H
#define MAGNIFYOPTIONS_H

// Qt
#include <QWidget>
// Local
#include "external/qxtSlider/qxtspanslider.h"
#include "main/other/Structures.h"
#include "main/other/Config.h"

namespace Ui {
class MagnifyOptions;
}

class MagnifyOptions : public QWidget
{
    Q_OBJECT

public:
    explicit MagnifyOptions(QWidget *parent = 0);
    ~MagnifyOptions();
    ImageProcessingSettings getSettings();
    ImageProcessingFlags getFlags();
    void toggleGrayscale(bool isActive);
    void setFPS(double fps);

private:
    Ui::MagnifyOptions *ui;
    QxtSpanSlider *doubleSlider;
    ImageProcessingSettings imgProcSettings;
    ImageProcessingFlags imgProcFlags;

public slots:
    void setMaxLevel(int level);
    void reset();

private slots:
    void updateFlagsFromOptionsTab();
    void updateSettingsFromOptionsTab();
    void reset(int);
    // Internal slots supporting GUI
    void convertFromSpinBox(double val);
    void convertFromSlider(int val);
    void applyColorInterface();
    void applyLaplaceInterface();
    void applyRieszInterface();

signals:
    void newImageProcessingFlags(struct ImageProcessingFlags);
    void newImageProcessingSettings(struct ImageProcessingSettings);


};

#endif // MAGNIFYOPTIONS_H
