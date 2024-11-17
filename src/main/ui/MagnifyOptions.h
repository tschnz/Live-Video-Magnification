#pragma once

// Qt
#include <QWidget>
// Local
#include "../../external/qxtSlider/qxtspanslider.h"
#include "../other/Config.h"
#include "../other/Structures.h"

namespace Ui {
class MagnifyOptions;
}

class MagnifyOptions : public QWidget {
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