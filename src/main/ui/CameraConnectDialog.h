/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->CameraConnectDialog.h                              */
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

#ifndef CAMERACONNECTDIALOG_H
#define CAMERACONNECTDIALOG_H

// Qt
#include <QDialog>
#include <QFileDialog>
#include <QtCore/QThread>
#include <QMessageBox>
#include <QDebug>
// Local
#include "main/other/Config.h"
// OpenCV
#include <opencv2/highgui/highgui.hpp>

namespace Ui {
class CameraConnectDialog;
}

class CameraConnectDialog : public QDialog
{
    Q_OBJECT
    
    public:
        explicit CameraConnectDialog(QWidget *parent=0);
        ~CameraConnectDialog();
        void setDeviceNumber();
        void setImageBufferSize();
        int getDeviceNumber();
        int getResolutionWidth();
        int getResolutionHeight();
        int getFpsNumber();
        int getImageBufferSize();
        bool getDropFrameCheckBoxState();
        bool getPgDevCheckBoxState();
        int getCaptureThreadPrio();
        int getProcessingThreadPrio();
        int getPlayerThreadPrio();
        bool isFile();
        bool isCamera();
        QString getFilepath();
        QString getTabLabel();

    private:
        Ui::CameraConnectDialog *ui;
        int countCameraDevices();

    public slots:
        void resetToDefaults();

    private slots:
        void toggleCameraGroupBox(bool state);
        void toggleFileGroupBox(bool state);
        void openButton_clicked();
};

#endif // CAMERACONNECTDIALOG_H
