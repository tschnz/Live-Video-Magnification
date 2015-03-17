/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->MainWindow.h                                       */
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QPushButton>
#include <QShortcut>
#include <QLabel>
#include <QMessageBox>
#include <QUrl>
// Local
#include "main/ui/CameraConnectDialog.h"
#include "main/ui/CameraView.h"
#include "main/ui/VideoView.h"
#include "main/other/Buffer.h"
#include "main/helper/SharedImageBuffer.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private:
        Ui::MainWindow *ui;
        QPushButton *connectToCameraButton;
        QPushButton *hideSettingsButton;
        QMap<int, int> deviceNumberMap;
        QMap<int, CameraView*> cameraViewMap;
        QMap<QString, int> fileNumberMap;
        QMap<QString, VideoView*> videoViewMap;
        SharedImageBuffer *sharedImageBuffer;
        bool removeFromMapByTabIndex(QMap<int, int>& map, int tabIndex);
        void updateMapValues(QMap<int, int>& map, int tabIndex);
        bool removeFromMapByTabIndex(QMap<QString, int>& map, int tabIndex);
        void updateMapValues(QMap<QString, int>& map, int tabIndex);
        void setTabCloseToolTips(QTabWidget *tabs, QString tooltip);
        void addCodecs();
        //Codecs
        int saveCodec;
        bool useVideoCodec;

    public slots:
        void connectToCamera();
        void disconnectCamera(int index);
        void showAboutDialog();
        void showAboutQtDialog();
        void showHelpDialog();
        void setFullScreen(bool);
        void setCodec(QAction* action);
};

#endif // MAINWINDOW_H
