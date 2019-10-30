/************************************************************************************/
/* An OpenCV/Qt based realtime application to magnify motion and color              */
/* Copyright (C) 2015  Jens Schindel <kontakt@jens-schindel.de>                     */
/*                                                                                  */
/* Based on the work of                                                             */
/*      Joseph Pan      <https://github.com/wzpan/QtEVM>                            */
/*      Nick D'Ademo    <https://github.com/nickdademo/qt-opencv-multithreaded>     */
/*                                                                                  */
/* Realtime-Video-Magnification->MainWindow.cpp                                     */
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


#include "main/ui/MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Setup UI
    ui->setupUi(this);
    // Set start tab as blank
    QLabel *newTab = new QLabel(ui->tabWidget);
    newTab->setText(tr("Neither camera connected nor file loaded."));
    newTab->setAlignment(Qt::AlignCenter);
    ui->tabWidget->addTab(newTab, "");
    ui->tabWidget->setTabsClosable(false);
    // Add "Connect to Camera" button to tab
    connectToCameraButton = new QPushButton();
    connectToCameraButton->setText(tr("Connect/Open"));
    ui->tabWidget->setCornerWidget(connectToCameraButton, Qt::TopLeftCorner);
    connect(connectToCameraButton,SIGNAL(released()),this, SLOT(connectToCamera()));
    connect(ui->tabWidget,SIGNAL(tabCloseRequested(int)),this, SLOT(disconnectCamera(int)));
    // Set focus on button
    connectToCameraButton->setFocus();
    connectToCameraButton->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_O));
    // Connect other signals/slots
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(showHelpDialog()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), this, SLOT(showAboutQtDialog()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionFullScreen, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));
    connect(ui->actionConnect_Open, SIGNAL(triggered()), this, SLOT(connectToCamera()));
    // Create SharedImageBuffer object
    sharedImageBuffer = new SharedImageBuffer();

    addCodecs();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectToCamera()
{
    // Get next tab index
    int nextTabIndex = (deviceNumberMap.size()+fileNumberMap.size()==0) ? 0 : ui->tabWidget->count();
    // Show dialog
    CameraConnectDialog *cameraConnectDialog = new CameraConnectDialog(this);
    if(cameraConnectDialog->exec()==QDialog::Accepted)
    {
        if(cameraConnectDialog->isCamera()) {
            // Save user-defined device number
            int deviceNumber = cameraConnectDialog->getDeviceNumber();
            // Check if this camera is already connected
            if(!deviceNumberMap.contains(deviceNumber))
            {
                // Create ImageBuffer with user-defined size
                Buffer<Mat> *imageBuffer = new Buffer<Mat>(cameraConnectDialog->getImageBufferSize());
                // Add created ImageBuffer to SharedImageBuffer object
                sharedImageBuffer->add(deviceNumber, imageBuffer);
                // Create CameraView
                cameraViewMap[deviceNumber] = new CameraView(ui->tabWidget, deviceNumber, sharedImageBuffer);
                // Attempt to connect to camera
                if(cameraViewMap[deviceNumber]->connectToCamera(cameraConnectDialog->getDropFrameCheckBoxState(),
                                               cameraConnectDialog->getCaptureThreadPrio(),
                                               cameraConnectDialog->getProcessingThreadPrio(),
                                               cameraConnectDialog->getResolutionWidth(),
                                               cameraConnectDialog->getResolutionHeight(),
                                               cameraConnectDialog->getFpsNumber()))
                {
                    // Add to map
                    deviceNumberMap[deviceNumber] = nextTabIndex;
                    // Save tab label
                    QString tabLabel = cameraConnectDialog->getTabLabel();
                    // Allow tabs to be closed
                    ui->tabWidget->setTabsClosable(true);
                    // If start tab, remove
                    if(nextTabIndex==0)
                        ui->tabWidget->removeTab(0);
                    // Add tab
                    ui->tabWidget->addTab(cameraViewMap[deviceNumber], tabLabel + " [" + QString::number(deviceNumber) + "]");
                    ui->tabWidget->setCurrentWidget(cameraViewMap[deviceNumber]);
                    // Set tooltips
                    setTabCloseToolTips(ui->tabWidget, tr("Disconnect Camera"));
                    //Set Codec
                    cameraViewMap[deviceNumber]->setCodec(saveCodec);
                }
                // Could not connect to camera
                else
                {
                    // Display error message
                    QMessageBox::warning(this,tr("ERROR:"),tr("Could not connect to camera. Please check device number."));
                    // Explicitly delete widget
                    delete cameraViewMap[deviceNumber];
                    cameraViewMap.remove(deviceNumber);
                    // Remove from shared buffer
                    sharedImageBuffer->removeByDeviceNumber(deviceNumber);
                    // Explicitly delete ImageBuffer object
                    delete imageBuffer;
                }
            }
            // Display error message
            else
                QMessageBox::warning(this,tr("ERROR:"),tr("Could not connect to camera. Already connected."));
        }
        // Attempt to load a file
        else if(cameraConnectDialog->isFile()) {
            // Get the filepath from connectDialog
            QString filepath = cameraConnectDialog->getFilepath();
            // String can't be empty
            if(filepath.isEmpty()) {
                QMessageBox::warning(this,tr("ERROR:"),tr("Please enter path to video file."));
                connectToCamera();
                return;
            }
            // Load the File
            QFileInfo file(filepath);
            QString filename = file.fileName();
            if(file.exists()){
                // Create new Videofile entry in QMap
                videoViewMap[filename] = new VideoView(ui->tabWidget,filepath);
                // Attemp to load the video
                if(videoViewMap[filename]->loadVideo(cameraConnectDialog->getPlayerThreadPrio(),
                                                     cameraConnectDialog->getResolutionWidth(),
                                                     cameraConnectDialog->getResolutionHeight(),
                                                     cameraConnectDialog->getFpsNumber()))
                {
                    // Add to map
                    fileNumberMap[filename] = nextTabIndex;
                    // Save tab label
                    QString tabLabel = cameraConnectDialog->getTabLabel();
                    // Allow tabs to be closed
                    ui->tabWidget->setTabsClosable(true);
                    // If start tab remove
                    if(nextTabIndex==0)
                        ui->tabWidget->removeTab(0);
                    // Add tab
                    ui->tabWidget->addTab(videoViewMap[filename], tabLabel+"["+filename+"]");
                    ui->tabWidget->setCurrentWidget(videoViewMap[filename]);
                    // Set tooltips
                    setTabCloseToolTips(ui->tabWidget, tr("Remove Video"));
                    // Set Codec
                    videoViewMap[filename]->setCodec(saveCodec);
                    videoViewMap[filename]->set_useVideoCodec(useVideoCodec);
                }
                // Could not load video
                else {
                    // Display error message
                    QMessageBox::warning(this,tr("ERROR:"),tr("Could not convert Video. Please check file format."));
                    // Explicitly delete widget
                    delete videoViewMap[filename];
                    videoViewMap.remove(filename);
                }
            }
            // Error if file does not exist
            else {
                QMessageBox::warning(this,tr("ERROR:"),tr("File does not exist."));
                connectToCamera();
                return;
            }
        }
    }
    // Delete dialog
    delete cameraConnectDialog;
}

void MainWindow::disconnectCamera(int index)
{
    // Save number of tabs
    int nTabs = ui->tabWidget->count();

    bool isCamera = (ui->tabWidget->widget(index)->objectName() == "CameraView");

    // Close tab
    ui->tabWidget->removeTab(index);
    // Delete widget contained in tab
    if(isCamera) {
        int camToDelete = deviceNumberMap.key(index);
        delete cameraViewMap[camToDelete];
        delete  sharedImageBuffer->getByDeviceNumber(camToDelete);
        cameraViewMap.remove(camToDelete);
    }
    else {
        QString vidToDelete = fileNumberMap.key(index);
        delete videoViewMap[vidToDelete];
        videoViewMap.remove(vidToDelete);
    }

    // Remove from map
    removeFromMapByTabIndex(deviceNumberMap, index);
    removeFromMapByTabIndex(fileNumberMap, index);

    // Update map (if tab closed is not last)
    if(index!=(nTabs-1)) {
        updateMapValues(deviceNumberMap, index);
        updateMapValues(fileNumberMap, index);
    }

    // If start tab, set tab as blank
    if(nTabs==1)
    {
        QLabel *newTab = new QLabel(ui->tabWidget);
        newTab->setText(tr("Neither camera connected nor file loaded."));
        newTab->setAlignment(Qt::AlignCenter);
        ui->tabWidget->addTab(newTab, "");
        ui->tabWidget->setTabsClosable(false);
    }
}

void MainWindow::showAboutDialog()
{
    QString aboutMessage;
    QTextStream ts(&aboutMessage);
    ts << "<h3>Realtime Video Magnification</h3>"<<
          "<p>This is an OpenCV and Qt based realtime application to magnify motion and color in videos and camerastreams.</p>"<<
          "<p>Copyright &copy; 2015 Jens Schindel</p>"<<
          "<p>The provided code is licensed under GPLv3 and hosted at "<<
          "<a href='https://github.com/tschnz/Realtime-Video-Magnification'>GitHub</a>.</p>"<<
          "<p>This application is based on the work of <a href='https://github.com/wzpan/QtEVM'>Joseph Pan</a> and "<<
          "<a href='https://github.com/nickdademo/qt-opencv-multithreaded'>Nick D&#39;Ademo</a>.</p>"<<
          "<p>The underlying algorithm is part of a research project originated at the MIT, called "<<
          "<a href='http://people.csail.mit.edu/mrub/vidmag/'>Eulerian Video Magnification</a></p>";

    QMessageBox::about(this, tr("About"), aboutMessage);
}

void MainWindow::showHelpDialog()
{
    QString helpMessage;
    QTextStream ts(&helpMessage);
    ts << "<p>The best way to understand is experimenting with the sliders. Additionally there are tooltips for every option, "<<
          "appearing while hovering over the labels/texts to the left of every slider/spinbox/checkbox.</p><p>If you should encounter "<<
          "crashs while color magnifying, try a lower resolution or change ROI (draw a box in the frame label).</p>"<<
          "<p>If you want to understand how this algorithm works please visit the MITs "<<
          "<a href='http://people.csail.mit.edu/mrub/vidmag/'>Eulerian Video Magnification</a> research page.</p>";

    QMessageBox::information(this, tr("Help"), helpMessage);
}

void MainWindow::showAboutQtDialog()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

bool MainWindow::removeFromMapByTabIndex(QMap<int, int> &map, int tabIndex)
{
    QMutableMapIterator<int, int> i(map);
    while (i.hasNext())
    {
         i.next();
         if(i.value()==tabIndex)
         {
             i.remove();
             return true;
         }
    }
    return false;
}

void MainWindow::updateMapValues(QMap<int, int> &map, int tabIndex)
{
    QMutableMapIterator<int, int> i(map);
    while (i.hasNext())
    {
        i.next();
        if(i.value()>tabIndex)
            i.setValue(i.value()-1);
    }
}

bool MainWindow::removeFromMapByTabIndex(QMap<QString, int> &map, int tabIndex)
{
    QMutableMapIterator<QString, int> i(map);
    while (i.hasNext())
    {
         i.next();
         if(i.value()==tabIndex)
         {
             i.remove();
             return true;
         }
    }
    return false;
}

void MainWindow::updateMapValues(QMap<QString, int> &map, int tabIndex)
{
    QMutableMapIterator<QString, int> i(map);
    while (i.hasNext())
    {
        i.next();
        if(i.value()>tabIndex)
            i.setValue(i.value()-1);
    }
}

void MainWindow::setTabCloseToolTips(QTabWidget *tabs, QString tooltip)
{
    QList<QAbstractButton*> allPButtons = tabs->findChildren<QAbstractButton*>();
    for (int ind = 0; ind < allPButtons.size(); ind++)
    {
        QAbstractButton* item = allPButtons.at(ind);
        if (item->inherits("CloseButton"))
            item->setToolTip(tooltip);
    }
}

void MainWindow::setFullScreen(bool input)
{
    if(input)
        this->showFullScreen();
    else
        this->showNormal();
}

void MainWindow::addCodecs()
{
    QMenu *codecMenu;
    QActionGroup *codecGroup;
    QAction *codecAction;

    //Set up Menu and action group
    codecMenu = new QMenu(tr("Set Saving Codec"), ui->menuFile);
    codecGroup = new QActionGroup(this);
    codecGroup->setExclusive(true);

    //Add codecs to menu and actionGroup
    codecAction = codecMenu->addAction(tr("Ask me (Windows)"));
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    //Set DIVX Standard
    codecAction = codecMenu->addAction("DivX");
    codecAction->setCheckable(true);
    codecAction->setChecked(true);
    setCodec(codecAction);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("FFV1 (FFMPEG Codec)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("HDYC (Raw YUV 4:2:2)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("HEVC (H.265)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("M4S2 (MPEG-4 v2)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("MJPG (Motion JPEG)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("MP2V (MPEG-2)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("MP4V (MPEG-4)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("MPEG (MPEG-1?)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addAction("PIM1 (MPEG-1)");
    codecAction->setCheckable(true);
    codecGroup->addAction(codecAction);

    codecAction = codecMenu->addSection(tr("For Videos"));
    codecAction = codecMenu->addAction(tr("Same as Videosource"));
    codecAction->setCheckable(true);
    codecAction->setChecked(true);
    useVideoCodec = true;

    //Connect and add to MenuBar
    connect(codecMenu, SIGNAL(triggered(QAction*)), this, SLOT(setCodec(QAction*)));
    ui->menuFile->insertMenu(ui->actionQuit,codecMenu);
}

void MainWindow::setCodec(QAction *action)
{
    QString name = action->text();
    int codec;

    if(name == "Same as Videosource") {
        QMap<QString, VideoView*>::iterator h;
        useVideoCodec = action->isChecked();

        for(h = videoViewMap.begin() ; h != videoViewMap.end(); h++)
            h.value()->set_useVideoCodec(useVideoCodec);
    }
    else {
        action->setChecked(true);

        if(name == "DivX")
            codec = VideoWriter::fourcc('D','I','V','X');
        else if(name == "FFV1 (FFMPEG Codec)")
            codec = VideoWriter::fourcc('F','F','V','1');
        else if(name == "HDYC (Raw YUV 4:2:2)")
            codec = VideoWriter::fourcc('H','D','Y','C');
        else if(name == "HEVC (H.265)")
            codec = VideoWriter::fourcc('H','E','V','C');
        else if(name == "M4S2 (MPEG-4 v2)")
            codec = VideoWriter::fourcc('M','4','S','2');
        else if(name == "MJPG (Motion JPEG)")
            codec = VideoWriter::fourcc('M','J','P','G');
        else if(name == "MP2V (MPEG-2)")
            codec = VideoWriter::fourcc('M','P','2','V');
        else if(name == "MP4V (MPEG-4)")
            codec = VideoWriter::fourcc('M','P','4','V');
        else if(name == "MPEG (MPEG-1?)")
            codec = VideoWriter::fourcc('M','P','E','G');
        else if(name == "PIM1 (MPEG-1)")
            codec = VideoWriter::fourcc('P','I','M','1');
        else
            codec = -1;

        QMap<int, CameraView*>::iterator i;
        for(i = cameraViewMap.begin() ; i != cameraViewMap.end(); i++)
            i.value()->setCodec(codec);

        QMap<QString, VideoView*>::iterator j;
        for(j = videoViewMap.begin() ; j != videoViewMap.end(); j++)
            j.value()->setCodec(codec);

        saveCodec = codec;
    }
}
