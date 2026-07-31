#pragma once

#include <QWidget>

#include "processing/IProcessor.hpp" // MagnificationMode, MagnificationParams

class QEvent;
class QGroupBox;
class QPushButton;

namespace livim {

class MagnificationControls;
class SegmentedControl;
class ToggleSwitch;

// Right-hand processing inspector. Source-agnostic: emits intent signals MainWindow forwards to
// PlaybackController, and exposes setters the window calls to reflect source state.
class ProcessingPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProcessingPanel(QWidget* parent = nullptr);

    // Disabled for an already single-channel source.
    void setGrayscaleAvailable(bool available);

    // Caps the Levels control to what the source resolution supports.
    void setMaxLevels(int maxLevels);

    // Clamps the Hz cutoffs to Nyquist (fps/2).
    void setCaptureFps(double fps);

    // Shows/hides the "Reset ROI" button.
    void setRoiActive(bool active);

    // Reflects the "Select ROI" toggle state without re-emitting.
    void setRoiSelecting(bool selecting);

signals:
    void grayscaleToggled(bool enabled);
    void magnificationChanged(MagnificationParams params);
    void downscaleChanged(int divisor); // 1 / 2 / 4 / 8
    void roiSelectModeChanged(bool selecting);
    void roiResetRequested();

protected:
    void changeEvent(QEvent* event) override;

private:
    void refreshIcons();

    ToggleSwitch*     grayscaleSwitch_ = nullptr;
    SegmentedControl* resolutionSeg_ = nullptr;
    QPushButton*      roiSelectButton_ = nullptr;
    QPushButton*      roiResetButton_ = nullptr;

    QGroupBox*             magGroup_ = nullptr;
    MagnificationControls* magControls_ = nullptr;
};

} // namespace livim
