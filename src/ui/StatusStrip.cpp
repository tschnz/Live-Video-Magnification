#include "ui/StatusStrip.hpp"

#include <algorithm>

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStyle>

#include "ui/Theme.hpp"

namespace livim {
namespace {

constexpr int kCellSpacing = 6; // px between caption / dot / value within a cell

} // namespace

StatusStrip::StatusStrip(QWidget* parent) : QWidget(parent) {
    setObjectName("statusStrip");
    setAttribute(Qt::WA_StyledBackground, true);

    captionFont_ = font();
    captionFont_.setPointSizeF(captionFont_.pointSizeF() * 0.80);
    captionFont_.setCapitalization(QFont::AllUppercase);
    captionFont_.setLetterSpacing(QFont::PercentageSpacing, 106.0);
    captionFont_.setWeight(QFont::DemiBold);

    valueFont_ = font();
    valueFont_.setFamilies({"DejaVu Sans Mono", "Cascadia Code", "Consolas", "Menlo"});
    valueFont_.setStyleHint(QFont::Monospace);
    valueFont_.setWeight(QFont::Medium);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(metrics::space4, 4, metrics::space4, 4);
    row->setSpacing(0);

    // FPS cell:  FPS  ●  <measured rate>  /  [playback input]
    speed_.root = new QWidget(this);
    auto* sl = new QHBoxLayout(speed_.root);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->setSpacing(kCellSpacing);

    auto* cap = new QLabel("Fps", speed_.root);
    cap->setObjectName("statCaption");
    cap->setFont(captionFont_);

    speed_.dot = new QLabel(speed_.root);
    speed_.dot->setObjectName("statDot");

    speed_.value = new QLabel(speed_.root);
    speed_.value->setObjectName("statValue");
    speed_.value->setFont(valueFont_);
    speed_.value->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    slash_ = new QLabel(QStringLiteral("/"), speed_.root);
    slash_->setObjectName("statSlash");

    playbackSpin_ = new QDoubleSpinBox(speed_.root);
    playbackSpin_->setObjectName("statSpin");
    playbackSpin_->setRange(1.0, 999.99);
    playbackSpin_->setDecimals(2);
    playbackSpin_->setSingleStep(1.0);
    playbackSpin_->setValue(30.0);
    playbackSpin_->setKeyboardTracking(false); // apply on Enter/focus-out, not per keystroke
    playbackSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    playbackSpin_->setFont(valueFont_);
    playbackSpin_->setToolTip(
        "Playback speed — the cadence the clip plays back at (and the timeline's wall-clock time). "
        "Separate from Capture FPS in the magnification panel, which is the footage's true rate used "
        "for the Hz maths.");

    // A camera is never re-timed, so its rate is shown read-only.
    reported_ = new QLabel(speed_.root);
    reported_->setObjectName("statValue");
    reported_->setFont(valueFont_);
    reported_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    reported_->setToolTip("The frame rate the camera reports delivering.");

    const QFontMetrics vfm(valueFont_);
    playbackSpin_->setFixedWidth(vfm.horizontalAdvance(QStringLiteral("999.99")) + 18);

    sl->addWidget(cap);
    sl->addWidget(speed_.dot);
    sl->addWidget(speed_.value);
    sl->addWidget(slash_);
    sl->addWidget(playbackSpin_);
    sl->addWidget(reported_);

    connect(playbackSpin_, &QDoubleSpinBox::valueChanged, this, &StatusStrip::playbackFpsChanged);

    row->addWidget(speed_.root);

    row->addStretch(1);

    hint_ = new QLabel(this);
    hint_->setObjectName("statHint");
    hint_->setMinimumWidth(0);
    hint_->setVisible(false);
    row->addWidget(hint_);

    notice_ = new QLabel(this);
    notice_->setObjectName("statNotice");
    notice_->setMinimumWidth(0);
    notice_->setVisible(false);
    row->addWidget(notice_);

    setStats(StatsSnapshot{}, 0.0, false, false);
}

void StatusStrip::applyState(QWidget* w, Health h) {
    const char* s = h == Health::Idle ? "idle" : h == Health::Ok ? "ok" : h == Health::Warn ? "warn" : "bad";
    if (w->property("state").toString() == QLatin1String(s)) return;
    w->setProperty("state", s);
    w->style()->unpolish(w);
    w->style()->polish(w);
}

void StatusStrip::setCell(Cell& c, Health h, const QString& value) {
    if (c.value->text() != value) c.value->setText(value);
    applyState(c.dot, h);
    applyState(c.value, h);
}

void StatusStrip::setStats(const StatsSnapshot& s, double targetFps, bool hasSource,
                           bool cameraSource) {
    // Paused or just-opened reads as idle, not live.
    const bool live = hasSource && s.fps > 0.05;

    statushealth::Inputs in;
    in.live = live;
    in.camera = cameraSource;
    in.fps = s.fps;
    in.targetFps = targetFps;
    in.dropFraction = s.dropFraction;

    const Health speedH = statushealth::speed(in);

    setCell(speed_, speedH,
            live ? QString::number(std::min(s.fps, 999.9), 'f', 1) : QStringLiteral("—"));

    const bool showInput = hasSource && !cameraSource;
    const bool showReported = hasSource && cameraSource && targetFps > 0.0;
    playbackSpin_->setVisible(showInput);
    reported_->setVisible(showReported);
    slash_->setVisible(showInput || showReported);
    if (showReported) {
        const QString t = QString::number(std::min(targetFps, 999.9), 'f', 1);
        if (reported_->text() != t) reported_->setText(t);
    }

    static const QString kAdvice =
        QStringLiteral("Falling behind — shrink the ROI or increase downscale");
    const bool strained = speedH == Health::Bad;
    if (strained) {
        if (hint_->text() != kAdvice) hint_->setText(kAdvice);
        hint_->setVisible(true);
    } else {
        hint_->setVisible(false);
    }
}

void StatusStrip::setPlaybackFps(double fps) {
    if (fps <= 0.0) return;
    const QSignalBlocker block(*playbackSpin_);
    playbackSpin_->setValue(fps);
}

void StatusStrip::showNotice(const QString& text) {
    if (notice_->text() != text) notice_->setText(text);
    notice_->setVisible(!text.isEmpty());
}

} // namespace livim
