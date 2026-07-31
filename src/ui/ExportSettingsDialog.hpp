#pragma once

#include <QDialog>
#include <QString>

#include "export/ExportTypes.hpp"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace livim {

class MagnificationControls;
class SegmentedControl;
class ToggleSwitch;

// Modal export-settings dialog, pre-filled with the processing panel's current values.
class ExportSettingsDialog : public QDialog {
    Q_OBJECT
public:
    struct Seed {
        MagnificationParams magnification;     // includes the Capture FPS as framerate
        bool                grayscale = false;
        bool                grayscaleAvailable = true;
        PreprocessParams    preprocess;
        int                 maxLevels = 0;     // caps the Levels control (0 = unknown)
        double              fileFpsDefault = 30.0;
        bool                isCamera = false;  // hide Start/End for a camera capture
        int                 frameCount = 0;    // total frames (for the Start/End ranges)
        int                 inFrame = 0;       // timeline in-point
        int                 outFrame = -1;     // timeline out-point; -1 = end
        QString             suggestedBaseName; // filename stem, no extension
    };

    explicit ExportSettingsDialog(const Seed& seed, QWidget* parent = nullptr);

    ExportRequest request() const { return request_; } // valid after exec() == Accepted

private:
    void browse();
    void syncOverlayEnabled();
    void updateApplyEnabled();
    void accept() override;

    Seed          seed_;
    ExportRequest request_;

    MagnificationControls* magControls_ = nullptr;
    ToggleSwitch*     grayscaleSwitch_ = nullptr;
    ToggleSwitch*     useRoiSwitch_ = nullptr;   // off = export the full frame
    SegmentedControl* resolutionSeg_ = nullptr;
    QDoubleSpinBox*   fileFpsSpin_ = nullptr;
    QComboBox*        splitCombo_ = nullptr;
    ToggleSwitch*     overlaySwitch_ = nullptr;   // split modes only
    QComboBox*        formatCombo_ = nullptr;
    QSpinBox*         startSpin_ = nullptr; // file only
    QSpinBox*         endSpin_ = nullptr;
    QLineEdit*        pathEdit_ = nullptr;
    QPushButton*      applyButton_ = nullptr;
};

} // namespace livim
