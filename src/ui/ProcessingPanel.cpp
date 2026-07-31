#include "ui/ProcessingPanel.hpp"

#include <algorithm>

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "ui/Icons.hpp"
#include "ui/MagnificationControls.hpp"
#include "ui/SegmentedControl.hpp"
#include "ui/Theme.hpp"
#include "ui/ToggleSwitch.hpp"

namespace livim {
namespace {

QLabel* fieldLabel(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setObjectName("fieldLabel");
    return l;
}

} // namespace

ProcessingPanel::ProcessingPanel(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(264);
    setMaximumWidth(440);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(metrics::space4, metrics::space4, metrics::space4, metrics::space4);
    layout->setSpacing(metrics::space3);

    auto* title = new QLabel("Processing", this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() + 1.0);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addWidget(fieldLabel("Resolution", this));
    resolutionSeg_ = new SegmentedControl(this);
    // Segment order must match kDivisors below.
    resolutionSeg_->addSegment("1/1");
    resolutionSeg_->addSegment("1/2");
    resolutionSeg_->addSegment("1/4");
    resolutionSeg_->addSegment("1/8");
    resolutionSeg_->setToolTip(
        "Process at a fraction of the source resolution. Lower means far less work, so the "
        "magnification can keep up with the source frame rate. The view is shown at the processed "
        "resolution.");
    layout->addWidget(resolutionSeg_);

    roiSelectButton_ = new QPushButton(" Select ROI", this);
    roiSelectButton_->setCheckable(true);
    roiSelectButton_->setIconSize(QSize(16, 16));
    roiSelectButton_->setToolTip(
        "Toggle ROI selection: turn it on, then drag a rectangle on the video to magnify only that "
        "region. It stays on so you can refine with another drag; toggle off to stop, or Reset ROI "
        "for the full frame.");
    roiResetButton_ = new QPushButton(" Reset ROI", this);
    roiResetButton_->setIconSize(QSize(16, 16));
    roiResetButton_->setToolTip("Clear the region of interest and process the full frame again.");
    roiResetButton_->setVisible(false); // shown only once an ROI is active
    auto* roiRow = new QWidget(this);
    auto* roiRowLayout = new QHBoxLayout(roiRow);
    roiRowLayout->setContentsMargins(0, 0, 0, 0);
    roiRowLayout->setSpacing(metrics::space2);
    roiRowLayout->addWidget(roiSelectButton_);
    roiRowLayout->addWidget(roiResetButton_);
    layout->addWidget(roiRow);

    auto* grayRow = new QWidget(this);
    auto* grayLayout = new QHBoxLayout(grayRow);
    grayLayout->setContentsMargins(0, 0, 0, 0);
    grayLayout->addWidget(new QLabel("Grayscale", grayRow));
    grayLayout->addStretch(1);
    grayscaleSwitch_ = new ToggleSwitch(grayRow);
    grayscaleSwitch_->setToolTip("Convert the incoming image to a single-channel grayscale texture.");
    grayLayout->addWidget(grayscaleSwitch_);
    layout->addWidget(grayRow);

    magGroup_ = new QGroupBox("Magnification", this);
    auto* magLayout = new QVBoxLayout(magGroup_);
    magLayout->setContentsMargins(metrics::space2, metrics::space2, metrics::space2, metrics::space2);
    magLayout->setSpacing(metrics::space2);
    magControls_ = new MagnificationControls(magGroup_);
    magLayout->addWidget(magControls_);
    layout->addWidget(magGroup_);

    layout->addStretch(1);

    refreshIcons();

    connect(grayscaleSwitch_, &ToggleSwitch::toggled, this, &ProcessingPanel::grayscaleToggled);
    connect(grayscaleSwitch_, &ToggleSwitch::toggled, magControls_, &MagnificationControls::setGrayscale);
    connect(resolutionSeg_, &SegmentedControl::currentIndexChanged, this, [this](int index) {
        static constexpr int kDivisors[] = {1, 2, 4, 8};
        emit downscaleChanged(kDivisors[std::clamp(index, 0, 3)]);
    });
    connect(roiSelectButton_, &QPushButton::toggled, this, &ProcessingPanel::roiSelectModeChanged);
    connect(roiResetButton_, &QPushButton::clicked, this, &ProcessingPanel::roiResetRequested);

    connect(magControls_, &MagnificationControls::magnificationChanged, this,
            &ProcessingPanel::magnificationChanged);
}

void ProcessingPanel::refreshIcons() {
    const QColor c = palette().color(QPalette::ButtonText);
    roiSelectButton_->setIcon(icons::roi(c, 16));
    roiResetButton_->setIcon(icons::reset(c, 16));
}

void ProcessingPanel::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) refreshIcons();
}

void ProcessingPanel::setGrayscaleAvailable(bool available) {
    if (grayscaleSwitch_->isEnabled() == available) return;
    grayscaleSwitch_->setEnabled(available);
    grayscaleSwitch_->setToolTip(
        available ? "Convert the incoming image to a single-channel grayscale texture."
                  : "The source already delivers a single channel — nothing to convert.");
}

void ProcessingPanel::setRoiActive(bool active) {
    roiResetButton_->setVisible(active);
}

void ProcessingPanel::setRoiSelecting(bool selecting) {
    const QSignalBlocker block(*roiSelectButton_);
    roiSelectButton_->setChecked(selecting);
}

void ProcessingPanel::setMaxLevels(int maxLevels) {
    magControls_->setMaxLevels(maxLevels);
}

void ProcessingPanel::setCaptureFps(double fps) {
    magControls_->setCaptureFps(fps);
}

} // namespace livim
