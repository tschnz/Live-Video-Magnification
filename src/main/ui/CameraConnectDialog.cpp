#include "CameraConnectDialog.h"
#include "ui_CameraConnectDialog.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>

CameraConnectDialog::CameraConnectDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CameraConnectDialog) {
  // Setup dialog
  ui->setupUi(this);
  // deviceNumberEdit (device number) input validation
  QRegularExpression rx1("^[0-9]{1,3}$"); // Integers 0 to 999
  QRegularExpressionValidator *validator1 =
      new QRegularExpressionValidator(rx1, 0);
  ui->deviceNumberEdit->setValidator(validator1);
  // imageBufferSizeEdit (image buffer size) input validation
  QRegularExpression rx2("^[0-9]{1,3}$"); // Integers 0 to 999
  QRegularExpressionValidator *validator2 =
      new QRegularExpressionValidator(rx2, 0);
  ui->imageBufferSizeEdit->setValidator(validator2);
  // resWEdit (resolution: width) input validation
  QRegularExpression rx3("^[0-9]{1,4}$"); // Integers 0 to 9999
  QRegularExpressionValidator *validator3 =
      new QRegularExpressionValidator(rx3, 0);
  ui->resWEdit->setValidator(validator3);
  // resHEdit (resolution: height) input validation
  QRegularExpression rx4("^[0-9]{1,4}$"); // Integers 0 to 9999
  QRegularExpressionValidator *validator4 =
      new QRegularExpressionValidator(rx4, 0);
  ui->resHEdit->setValidator(validator4);
  // fpsEdit input validation
  QRegularExpression rx5("^[0-9]{1,3}$"); // Integers 0 to 999
  QRegularExpressionValidator *validator5 =
      new QRegularExpressionValidator(rx5, 0);
  ui->fpsEdit->setValidator(validator5);
  // Setup combo boxes
  QStringList threadPriorities;
  threadPriorities << "Idle"
                   << "Lowest"
                   << "Low"
                   << "Normal"
                   << "High"
                   << "Highest"
                   << "Time Critical"
                   << "Inherit";
  ui->capturePrioComboBox->addItems(threadPriorities);
  ui->processingPrioComboBox->addItems(threadPriorities);
  ui->playerPrioComboBox->addItems(threadPriorities);
  // Set dialog to defaults
  resetToDefaults();
  // Connect button to slot
  connect(ui->resetToDefaultsPushButton, SIGNAL(released()),
          SLOT(resetToDefaults()));
  connect(ui->fileGroupBox, SIGNAL(clicked(bool)),
          SLOT(toggleCameraGroupBox(bool)));
  connect(ui->cameraGroupBox, SIGNAL(clicked(bool)),
          SLOT(toggleFileGroupBox(bool)));
  connect(ui->openButton, SIGNAL(released()), SLOT(openButton_clicked()));
}

CameraConnectDialog::~CameraConnectDialog() { delete ui; }

int CameraConnectDialog::getDeviceNumber() {
  int deviceNumber;

  // Set device number to default (any available camera) if field is blank
  if (ui->deviceNumberEdit->text().isEmpty()) {
    QMessageBox::warning(
        this->parentWidget(), tr("WARNING:"),
        tr("Device Number field blank.\nAutomatically set to 0."));
    deviceNumber = 0;
  } else
    deviceNumber = ui->deviceNumberEdit->text().toInt();

  return deviceNumber;
}

int CameraConnectDialog::countCameraDevices() {
  cv::VideoCapture tmp;
  bool isWorking;
  int count = 0;
  // Test i times if a device is reachable
  for (int i = 0; i <= 5; i++) {
    cv::VideoCapture tmp(i);
    isWorking = tmp.isOpened();
    tmp.release();
    if (isWorking)
      count++;
  }

  return count;
}

int CameraConnectDialog::getFpsNumber() {
  // Return -1 if field is blank
  return (ui->fpsEdit->text().isEmpty() ? -1 : ui->fpsEdit->text().toInt());
}

int CameraConnectDialog::getResolutionWidth() {
  // Return -1 if field is blank
  if (ui->resWEdit->text().isEmpty())
    return -1;
  else
    return ui->resWEdit->text().toInt();
}

int CameraConnectDialog::getResolutionHeight() {
  // Return -1 if field is blank
  if (ui->resHEdit->text().isEmpty())
    return -1;
  else
    return ui->resHEdit->text().toInt();
}

int CameraConnectDialog::getImageBufferSize() {
  // Set image buffer size to default if field is blank
  if (ui->imageBufferSizeEdit->text().isEmpty()) {
    QMessageBox::warning(this->parentWidget(), tr("WARNING:"),
                         tr("Image Buffer Size field blank.\nAutomatically set "
                            "to default value."));
    return DEFAULT_IMAGE_BUFFER_SIZE;
  }
  // Set image buffer size to default if field is zero
  else if (ui->imageBufferSizeEdit->text().toInt() == 0) {
    QMessageBox::warning(this->parentWidget(), tr("WARNING:"),
                         tr("Image Buffer Size cannot be zero.\nAutomatically "
                            "set to default value."));
    return DEFAULT_IMAGE_BUFFER_SIZE;
    ;
  }
  // Use image buffer size specified by user
  else
    return ui->imageBufferSizeEdit->text().toInt();
}

bool CameraConnectDialog::getDropFrameCheckBoxState() {
  return ui->dropFrameCheckBox->isChecked();
}

int CameraConnectDialog::getCaptureThreadPrio() {
  return ui->capturePrioComboBox->currentIndex();
}

int CameraConnectDialog::getProcessingThreadPrio() {
  return ui->processingPrioComboBox->currentIndex();
}

int CameraConnectDialog::getPlayerThreadPrio() {
  return ui->playerPrioComboBox->currentIndex();
}

QString CameraConnectDialog::getTabLabel() { return ui->tabLabelEdit->text(); }

void CameraConnectDialog::resetToDefaults() {
  // Default camera
  ui->deviceNumberEdit->clear();
  // Resolution
  ui->resWEdit->clear();
  ui->resHEdit->clear();
  // Image buffer size
  ui->imageBufferSizeEdit->setText(QString::number(DEFAULT_IMAGE_BUFFER_SIZE));
  // Drop frames
  ui->dropFrameCheckBox->setChecked(DEFAULT_DROP_FRAMES);
  // Capture thread
  if (DEFAULT_CAP_THREAD_PRIO == QThread::IdlePriority)
    ui->capturePrioComboBox->setCurrentIndex(0);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::LowestPriority)
    ui->capturePrioComboBox->setCurrentIndex(1);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::LowPriority)
    ui->capturePrioComboBox->setCurrentIndex(2);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::NormalPriority)
    ui->capturePrioComboBox->setCurrentIndex(3);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::HighPriority)
    ui->capturePrioComboBox->setCurrentIndex(4);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::HighestPriority)
    ui->capturePrioComboBox->setCurrentIndex(5);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::TimeCriticalPriority)
    ui->capturePrioComboBox->setCurrentIndex(6);
  else if (DEFAULT_CAP_THREAD_PRIO == QThread::InheritPriority)
    ui->capturePrioComboBox->setCurrentIndex(7);
  // Processing thread
  if (DEFAULT_PROC_THREAD_PRIO == QThread::IdlePriority)
    ui->processingPrioComboBox->setCurrentIndex(0);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::LowestPriority)
    ui->processingPrioComboBox->setCurrentIndex(1);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::LowPriority)
    ui->processingPrioComboBox->setCurrentIndex(2);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::NormalPriority)
    ui->processingPrioComboBox->setCurrentIndex(3);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::HighPriority)
    ui->processingPrioComboBox->setCurrentIndex(4);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::HighestPriority)
    ui->processingPrioComboBox->setCurrentIndex(5);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::TimeCriticalPriority)
    ui->processingPrioComboBox->setCurrentIndex(6);
  else if (DEFAULT_PROC_THREAD_PRIO == QThread::InheritPriority)
    ui->processingPrioComboBox->setCurrentIndex(7);
  // Player thread
  if (DEFAULT_PLAY_THREAD_PRIO == QThread::IdlePriority)
    ui->playerPrioComboBox->setCurrentIndex(0);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::LowestPriority)
    ui->playerPrioComboBox->setCurrentIndex(1);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::LowPriority)
    ui->playerPrioComboBox->setCurrentIndex(2);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::NormalPriority)
    ui->playerPrioComboBox->setCurrentIndex(3);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::HighPriority)
    ui->playerPrioComboBox->setCurrentIndex(4);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::HighestPriority)
    ui->playerPrioComboBox->setCurrentIndex(5);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::TimeCriticalPriority)
    ui->playerPrioComboBox->setCurrentIndex(6);
  else if (DEFAULT_PLAY_THREAD_PRIO == QThread::InheritPriority)
    ui->playerPrioComboBox->setCurrentIndex(7);
  // Tab label
  ui->tabLabelEdit->setText("");
  // FPS
  ui->fpsEdit->clear();
  // Filepath
  ui->fileSourceEdit->clear();
}

// Toggle the GroupBoxes, so either camer OR file is loaded
void CameraConnectDialog::toggleCameraGroupBox(bool state) {
  ui->cameraGroupBox->setChecked(!state);
}
void CameraConnectDialog::toggleFileGroupBox(bool state) {
  ui->fileGroupBox->setChecked(!state);
}

// Action to search for file via "Open" Button
void CameraConnectDialog::openButton_clicked() {
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open Video"), ".",
      tr("Video Files (*.avi *.wmv *.mov *.mpeg *.m4v *.mp4 *.mkv .*mts .*mpg "
         "*.AVI *.WMV *.MOV *.MPEG *.M4V *.MP4 *.MKV .*MTS .*MPG)"));

  if (!fileName.isEmpty()) {
    ui->fileSourceEdit->setText(fileName);
  }

  // Set Focus back on Ok Button
  ui->okCancelBox->button(QDialogButtonBox::Ok)->setFocus();
}

// Get the chosen filepath
QString CameraConnectDialog::getFilepath() {
  return ui->fileSourceEdit->text();
}

// Check whether User wants to open a File or CameraConnection
bool CameraConnectDialog::isFile() { return ui->fileGroupBox->isChecked(); }
bool CameraConnectDialog::isCamera() { return ui->cameraGroupBox->isChecked(); }
