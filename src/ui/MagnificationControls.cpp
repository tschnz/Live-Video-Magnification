#include "ui/MagnificationControls.hpp"

#include <algorithm>
#include <cmath>

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "processing/MagnificationParamsUi.hpp"
#include "ui/RangeSlider.hpp"
#include "ui/SliderRow.hpp"
#include "ui/Theme.hpp"

namespace livim {
namespace {

QWidget* makeRow(const QString& label, QWidget* w) {
    auto* row = new QWidget;
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(metrics::space1);
    l->addWidget(new QLabel(label, row));
    l->addStretch(1);
    l->addWidget(w);
    return row;
}

// Pull the leading number out of a readout string ("12.5 %" -> 12.5), tolerating a decimal comma.
double parseLeadingNumber(const QString& s, bool* ok) {
    static const QRegularExpression re(QStringLiteral("[-+]?[0-9]*[.,]?[0-9]+"));
    const QRegularExpressionMatch m = re.match(s);
    if (!m.hasMatch()) {
        if (ok) *ok = false;
        return 0.0;
    }
    return QString(m.captured(0)).replace(QLatin1Char(','), QLatin1Char('.')).toDouble(ok);
}

QLineEdit* makeValueEdit(Qt::Alignment align) {
    auto* e = new QLineEdit;
    e->setObjectName("valueReadout");
    e->setAlignment(align | Qt::AlignVCenter);
    // Measure in the mono font the #valueReadout QSS renders in, or the box under-sizes and clips.
    QFont f = e->font();
    f.setFamilies({"DejaVu Sans Mono", "Cascadia Code", "Consolas", "Menlo"});
    f.setPixelSize(11);
    e->setFixedWidth(QFontMetrics(f).horizontalAdvance(QStringLiteral("999.9 Hz")) + 20);
    return e;
}

// The value chip sits on the label row and mirrors the slider's formatted readout; the slider's
// own readout is hidden.
QWidget* makeSliderParam(const QString& label, SliderRow* slider) {
    slider->setReadoutVisible(false);
    auto* box = makeValueEdit(Qt::AlignRight);
    box->setText(slider->text());
    // Never stomp on the user while they are typing.
    QObject::connect(slider, &SliderRow::textChanged, box, [box](const QString& t) {
        if (!box->hasFocus()) box->setText(t);
    });
    // On commit the accepted, formatted value is reflected back into the box.
    QObject::connect(box, &QLineEdit::editingFinished, slider, [slider, box] {
        bool ok = false;
        const double v = parseLeadingNumber(box->text(), &ok);
        if (ok) slider->setValueUser(v);
        box->setText(slider->text());
    });

    auto* row = new QWidget;
    auto* v = new QVBoxLayout(row);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(metrics::space1);
    auto* head = new QHBoxLayout;
    head->setContentsMargins(0, 0, 0, 0);
    auto* l = new QLabel(label, row);
    l->setObjectName("fieldLabel");
    head->addWidget(l);
    head->addStretch(1);
    head->addWidget(box);
    v->addLayout(head);
    v->addWidget(slider);
    return row;
}

} // namespace

MagnificationControls::MagnificationControls(QWidget* parent) : QWidget(parent) {
    buildUi();
    applyModeDefaults(MagnificationMode::Laplace);
}

void MagnificationControls::buildUi() {
    auto* g = new QVBoxLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setSpacing(metrics::space3);

    magModeCombo_ = new QComboBox(this);
    magModeCombo_->setObjectName("valueReadout");
    // Item order must match MagnificationMode; the index is cast to it directly.
    magModeCombo_->addItem("Motion (Laplace)");
    magModeCombo_->addItem("Motion (Phase)");
    magModeCombo_->addItem("Color");
    magModeCombo_->setToolTip("Eulerian magnification algorithm. To turn magnification off, set the "
                              "top view selector to \"Show Original\".");

    captureFpsSpin_ = new QDoubleSpinBox(this);
    captureFpsSpin_->setObjectName("valueReadout");
    captureFpsSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    captureFpsSpin_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    captureFpsSpin_->setRange(1.0, 100000.0); // wide enough for high-speed cameras
    captureFpsSpin_->setDecimals(3);
    captureFpsSpin_->setSingleStep(1.0);
    captureFpsSpin_->setSuffix(" FPS");
    captureFpsSpin_->setKeyboardTracking(false);
    captureFpsSpin_->setValue(30.0);
    captureFpsSpin_->setToolTip(
        "True capture rate of the footage. Used only by the processing step (Colour/Riesz temporal "
        "filters and the Nyquist limit on the Hz cutoffs). Independent of playback / output speed.");

    // Shared width for Mode + Capture FPS, measured in the chip's mono font.
    QFont chipFont = magModeCombo_->font();
    chipFont.setFamilies({"DejaVu Sans Mono", "Cascadia Code", "Consolas", "Menlo"});
    chipFont.setPixelSize(11); // matches the #valueReadout QSS
    const int fieldW = QFontMetrics(chipFont).horizontalAdvance(QStringLiteral("Motion (Laplace)")) + 34;
    magModeCombo_->setFixedWidth(fieldW);
    captureFpsSpin_->setFixedWidth(fieldW);

    g->addWidget(makeRow("Mode", magModeCombo_));
    g->addWidget(makeRow("Capture FPS", captureFpsSpin_));

    amplificationSlider_ = new SliderRow(this);
    amplificationSlider_->setToolTip(
        "Amplification factor. The higher, the more the selected band is amplified "
        "(raise Cutoff Wavelength to keep noise down).");
    ampRow_ = makeSliderParam("Amplification", amplificationSlider_);
    g->addWidget(ampRow_);

    wavelengthSlider_ = new SliderRow(this);
    wavelengthSlider_->setToolTip(
        "Spatial cutoff wavelength. The higher, the less noise and small/short movements are amplified.");
    wavelengthRow_ = makeSliderParam("Cutoff Wavelength", wavelengthSlider_);
    g->addWidget(wavelengthRow_);

    // Temporal band: Hz chips and caption above the dual slider, BPM labels under the slider ends
    // (inset 9px to align under the chips' text).
    freqSlider_ = new RangeSlider(this);
    lowFreqEdit_ = makeValueEdit(Qt::AlignLeft);
    highFreqEdit_ = makeValueEdit(Qt::AlignRight);
    // Bad input restores the last good text.
    connect(lowFreqEdit_, &QLineEdit::editingFinished, this, [this] {
        bool ok = false;
        const double hz = parseLeadingNumber(lowFreqEdit_->text(), &ok);
        if (ok) { freqSlider_->setValues(hz, freqSlider_->highValue()); onSettingChanged(); }
        else refreshFreqReadouts();
    });
    connect(highFreqEdit_, &QLineEdit::editingFinished, this, [this] {
        bool ok = false;
        const double hz = parseLeadingNumber(highFreqEdit_->text(), &ok);
        if (ok) { freqSlider_->setValues(freqSlider_->lowValue(), hz); onSettingChanged(); }
        else refreshFreqReadouts();
    });
    lowBpmLabel_ = new QLabel(this);
    lowBpmLabel_->setObjectName("freqSubLabel");
    lowBpmLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    lowBpmLabel_->setContentsMargins(9, 0, 0, 0);
    highBpmLabel_ = new QLabel(this);
    highBpmLabel_->setObjectName("freqSubLabel");
    highBpmLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    highBpmLabel_->setContentsMargins(0, 0, 9, 0);

    freqRow_ = new QWidget(this);
    {
        auto* v = new QVBoxLayout(freqRow_);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(2);
        auto* r1 = new QHBoxLayout;
        r1->setContentsMargins(0, 0, 0, 0);
        auto* cap = new QLabel("Frequency", freqRow_);
        cap->setObjectName("fieldLabel");
        r1->addWidget(lowFreqEdit_);
        r1->addStretch(1);
        r1->addWidget(cap);
        r1->addStretch(1);
        r1->addWidget(highFreqEdit_);
        v->addLayout(r1);
        v->addWidget(freqSlider_);
        auto* r2 = new QHBoxLayout;
        r2->setContentsMargins(0, 0, 0, 0);
        r2->addWidget(lowBpmLabel_);
        r2->addStretch(1);
        r2->addWidget(highBpmLabel_);
        v->addLayout(r2);
    }
    freqRow_->setToolTip(
        "Temporal frequency band (lower / upper cutoff). Low values amplify slow movements, high "
        "values fast ones. Shown in Hz and beats per minute; the upper cutoff is capped at half the "
        "Capture FPS (Nyquist).");
    g->addWidget(freqRow_);

    chromSlider_ = new SliderRow(this);
    chromSlider_->setToolTip("Chrominance attenuation. The higher, the more colourful the movements.");
    chromRow_ = makeSliderParam("Chroma attenuation", chromSlider_);
    g->addWidget(chromRow_);

    levelsSlider_ = new SliderRow(this);
    levelsSlider_->setToolTip("Number of spatial-pyramid levels (capped by the source resolution).");
    levelsRow_ = makeSliderParam("Levels", levelsSlider_);
    g->addWidget(levelsRow_);

    resetButton_ = new QPushButton("Reset", this);
    resetButton_->setToolTip("Reset this mode's parameters to their defaults.");
    g->addWidget(resetButton_);

    connect(amplificationSlider_, &SliderRow::valueChanged, this, &MagnificationControls::onSettingChanged);
    connect(wavelengthSlider_, &SliderRow::valueChanged, this, &MagnificationControls::onSettingChanged);
    connect(freqSlider_, &RangeSlider::valuesChanged, this,
            [this](double, double) { onSettingChanged(); });
    connect(chromSlider_, &SliderRow::valueChanged, this, &MagnificationControls::onSettingChanged);
    connect(levelsSlider_, &SliderRow::valueChanged, this, &MagnificationControls::onSettingChanged);
    // Capture FPS moves the Nyquist limit, so re-clamp the Hz cutoffs before publishing. Saving and
    // restoring the guard keeps an outer silent seed from emitting mid-seed.
    connect(captureFpsSpin_, &QDoubleSpinBox::valueChanged, this, [this] {
        const bool wasUpdating = updating_;
        updating_ = true;
        configureModeUi(static_cast<MagnificationMode>(magModeCombo_->currentIndex()));
        updating_ = wasUpdating;
        if (!wasUpdating) onSettingChanged();
    });
    connect(magModeCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        applyModeDefaults(static_cast<MagnificationMode>(index));
    });
    connect(resetButton_, &QPushButton::clicked, this, [this] {
        applyModeDefaults(static_cast<MagnificationMode>(magModeCombo_->currentIndex()));
    });
}

// Ranges, suffixes, row visibility, Nyquist clamp and Levels cap only. Writes no values and emits
// nothing; the caller must hold updating_ true.
void MagnificationControls::configureModeUi(MagnificationMode mode) {
    const int levelCap = maxLevels_ > 0 ? maxLevels_ : 8;
    levelsSlider_->setRange(1.0, static_cast<double>(levelCap));
    levelsSlider_->setSingleStep(1.0);
    levelsSlider_->setDecimals(0);

    // The upper handle must stay below Nyquist (fps/2) or the temporal filter degenerates; the
    // 0.05 Hz floor keeps the low end off DC.
    const double fps = captureFpsSpin_->value();
    const double nyquist = (fps > 0.0 ? fps : 30.0) / 2.0;
    freqSlider_->setStep(0.01);
    freqSlider_->setRange(0.05, std::max(0.1, nyquist));
    freqSlider_->setLogScale(false);

    const bool none = (mode == MagnificationMode::None);
    ampRow_->setVisible(!none);
    levelsRow_->setVisible(!none);
    freqRow_->setVisible(!none);
    amplificationSlider_->setRange(0.0, 200.0);
    amplificationSlider_->setSingleStep(1.0);
    amplificationSlider_->setDecimals(0);
    amplificationSlider_->setSuffix(QString());

    switch (mode) {
    case MagnificationMode::Color:
        wavelengthRow_->setVisible(false);
        chromRow_->setVisible(false);
        break;
    case MagnificationMode::Laplace:
        wavelengthSlider_->setRange(0.0, 100.0);
        wavelengthSlider_->setSingleStep(1.0);
        wavelengthSlider_->setDecimals(1);
        wavelengthSlider_->setSuffix(" %");
        wavelengthRow_->setVisible(true);
        chromRow_->setVisible(!grayscale_); // no chrominance on a single-channel pipeline
        break;
    case MagnificationMode::Phase:
        wavelengthSlider_->setRange(0.0, 100.0);
        wavelengthSlider_->setSingleStep(1.0);
        wavelengthSlider_->setDecimals(1);
        wavelengthSlider_->setSuffix(QString());
        wavelengthRow_->setVisible(true);
        chromRow_->setVisible(false);
        break;
    case MagnificationMode::None:
        wavelengthRow_->setVisible(false);
        chromRow_->setVisible(false);
        break;
    }
    chromSlider_->setRange(0.0, 100.0);
    chromSlider_->setSingleStep(1.0);
    chromSlider_->setDecimals(0);
    chromSlider_->setSuffix(" %");
    resetButton_->setVisible(!none);
}

// Writes happen under the updating_ guard so per-widget signals don't publish intermediate states;
// one emit follows at the end.
void MagnificationControls::applyModeDefaults(MagnificationMode mode) {
    updating_ = true;
    configureModeUi(mode);

    const int levelCap = maxLevels_ > 0 ? maxLevels_ : 8;
    const MagUiValues d = defaultsFor(mode);

    amplificationSlider_->setValue(static_cast<double>(d.amplification));
    wavelengthSlider_->setValue(d.wavelength);
    freqSlider_->setValues(d.low, d.high);
    chromSlider_->setValue(static_cast<double>(d.chroma));
    levelsSlider_->setValue(static_cast<double>(std::min(d.levels, levelCap)));

    updating_ = false;
    refreshFreqReadouts();
    emit magnificationChanged(collectParams());
}

void MagnificationControls::setParams(const MagnificationParams& params) {
    const MagUiValues v = toUi(params);
    updating_ = true;
    {
        const QSignalBlocker block(*magModeCombo_); // don't trigger applyModeDefaults
        magModeCombo_->setCurrentIndex(static_cast<int>(v.mode));
    }
    captureFpsSpin_->setValue(v.captureFps > 0.0 ? v.captureFps : 30.0); // Nyquist, so set it first
    configureModeUi(v.mode);
    amplificationSlider_->setValue(static_cast<double>(v.amplification));
    levelsSlider_->setValue(static_cast<double>(v.levels));
    wavelengthSlider_->setValue(v.wavelength);
    freqSlider_->setValues(v.low, v.high);
    chromSlider_->setValue(static_cast<double>(v.chroma));
    updating_ = false;
    refreshFreqReadouts();
}

void MagnificationControls::onSettingChanged() {
    refreshFreqReadouts();
    if (updating_) return;
    emit magnificationChanged(collectParams());
}

void MagnificationControls::refreshFreqReadouts() {
    const double lo = freqSlider_->lowValue();
    const double hi = freqSlider_->highValue();
    lowFreqEdit_->setText(QString::number(lo, 'f', 1) + " Hz");
    highFreqEdit_->setText(QString::number(hi, 'f', 1) + " Hz");
    lowBpmLabel_->setText(QString("%1 BPM").arg(static_cast<int>(std::lround(lo * 60.0))));
    highBpmLabel_->setText(QString("%1 BPM").arg(static_cast<int>(std::lround(hi * 60.0))));
}

MagnificationParams MagnificationControls::collectParams() const {
    MagUiValues v;
    v.mode = static_cast<MagnificationMode>(magModeCombo_->currentIndex());
    v.amplification = static_cast<int>(amplificationSlider_->value());
    v.wavelength = wavelengthSlider_->value();
    v.low = freqSlider_->lowValue();
    v.high = freqSlider_->highValue();
    v.chroma = static_cast<int>(chromSlider_->value());
    v.levels = static_cast<int>(levelsSlider_->value());
    v.captureFps = captureFpsSpin_->value();
    return toParams(v);
}

void MagnificationControls::setMaxLevels(int maxLevels) {
    maxLevels_ = maxLevels;
    const int cap = maxLevels > 0 ? maxLevels : 8;
    updating_ = true;
    levelsSlider_->setRange(1.0, static_cast<double>(cap)); // SliderRow clamps the current value
    updating_ = false;
    // Re-emit so the processor gets the possibly clamped level count.
    emit magnificationChanged(collectParams());
}

void MagnificationControls::setCaptureFps(double fps) {
    updating_ = true;
    captureFpsSpin_->setValue(fps > 0.0 ? fps : 30.0);
    configureModeUi(static_cast<MagnificationMode>(magModeCombo_->currentIndex()));
    updating_ = false;
    refreshFreqReadouts(); // the band may have been clamped to the new Nyquist
    emit magnificationChanged(collectParams());
}

void MagnificationControls::setGrayscale(bool on) {
    if (grayscale_ == on) return;
    grayscale_ = on;
    // Save/restore updating_ so a range re-clamp inside configureModeUi cannot republish; grayscale
    // is published separately.
    const bool wasUpdating = updating_;
    updating_ = true;
    configureModeUi(static_cast<MagnificationMode>(magModeCombo_->currentIndex()));
    updating_ = wasUpdating;
}

} // namespace livim
