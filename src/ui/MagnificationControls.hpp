#pragma once

#include <QWidget>

#include "processing/IProcessor.hpp" // MagnificationMode, MagnificationParams

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace livim {

class SliderRow;
class RangeSlider;

// Magnification control cluster shared by the live processing panel and the export dialog: one
// source of truth for the per-mode UI, the Nyquist clamp, and the UI<->algorithm unit conversions.
class MagnificationControls : public QWidget {
    Q_OBJECT
public:
    explicit MagnificationControls(QWidget* parent = nullptr);

    // Inverse of collectParams. Silent: emits nothing, and the mode combo is set without
    // re-running the per-mode default writes.
    void setParams(const MagnificationParams& params);

    MagnificationParams collectParams() const;

    // Caps the Levels control to what the source resolution supports; re-emits.
    void setMaxLevels(int maxLevels);

    // Re-clamps the Hz cutoffs to the new Nyquist limit; re-emits.
    void setCaptureFps(double fps);

    // Hides the Chroma attenuation row on a single-channel pipeline. Structural only; emits nothing,
    // since grayscale is not part of MagnificationParams.
    void setGrayscale(bool on);

signals:
    void magnificationChanged(MagnificationParams params);

private:
    void buildUi();
    void configureModeUi(MagnificationMode mode);
    void applyModeDefaults(MagnificationMode mode);
    void onSettingChanged();
    void refreshFreqReadouts();

    QComboBox*      magModeCombo_ = nullptr;
    SliderRow*      amplificationSlider_ = nullptr;
    SliderRow*      wavelengthSlider_ = nullptr;
    RangeSlider*    freqSlider_ = nullptr; // dual-handle temporal band, Hz in every mode
    QLineEdit*      lowFreqEdit_ = nullptr;
    QLineEdit*      highFreqEdit_ = nullptr;
    QLabel*         lowBpmLabel_ = nullptr;
    QLabel*         highBpmLabel_ = nullptr;
    SliderRow*      chromSlider_ = nullptr;
    SliderRow*      levelsSlider_ = nullptr;
    QDoubleSpinBox* captureFpsSpin_ = nullptr; // drives Nyquist and the temporal filters
    QPushButton*    resetButton_ = nullptr;

    // Row containers; shown/hidden per mode so empty rows collapse.
    QWidget* ampRow_ = nullptr;
    QWidget* wavelengthRow_ = nullptr;
    QWidget* freqRow_ = nullptr;
    QWidget* chromRow_ = nullptr;
    QWidget* levelsRow_ = nullptr;

    bool updating_ = false;  // true while programmatically setting widgets (suppresses emits)
    int  maxLevels_ = 0;     // 0 = unknown (no source yet)
    bool grayscale_ = false; // single-channel pipeline -> hide Chroma attenuation
};

} // namespace livim
