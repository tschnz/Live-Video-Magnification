#include "ui/ExportSettingsDialog.hpp"

#include <algorithm>

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "ui/MagnificationControls.hpp"
#include "ui/SegmentedControl.hpp"
#include "ui/Theme.hpp" // metrics
#include "ui/ToggleSwitch.hpp"

namespace livim {
namespace {

constexpr int kDivisors[] = {1, 2, 4, 8};

int divisorIndex(int divisor) {
    for (int i = 0; i < 4; ++i)
        if (kDivisors[i] == divisor) return i;
    return 0;
}

QLabel* fieldLabel(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setObjectName("fieldLabel");
    return l;
}

QWidget* labeledRow(const QString& label, QWidget* control, QWidget* parent) {
    auto* row = new QWidget(parent);
    auto* l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(fieldLabel(label, row));
    l->addStretch(1);
    l->addWidget(control);
    return row;
}

} // namespace

ExportSettingsDialog::ExportSettingsDialog(const Seed& seed, QWidget* parent)
    : QDialog(parent), seed_(seed) {
    setWindowTitle("Export magnified video");
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(metrics::space3);

    QFont capFont = font();
    capFont.setPointSizeF(capFont.pointSizeF() * 0.80);
    capFont.setCapitalization(QFont::AllUppercase);
    capFont.setLetterSpacing(QFont::PercentageSpacing, 106.0);
    capFont.setWeight(QFont::DemiBold);

    // Shared width for the File FPS + Format fields: widest format label plus drop-down arrow room.
    const int fieldW = QFontMetrics(font()).horizontalAdvance(QStringLiteral("AVI (Motion JPEG)")) + 40;

    splitCombo_ = new QComboBox(this);
    splitCombo_->setFont(capFont);
    // Item order must match SplitMode; the index is cast to it directly.
    splitCombo_->addItem("None");
    splitCombo_->addItem("Side-by-Side");
    splitCombo_->addItem("Top-and-Bottom");
    splitCombo_->setToolTip("Write the original and processed frames together (side-by-side / stacked), "
                            "or the processed video only.");
    layout->addWidget(labeledRow("Split", splitCombo_, this));

    overlaySwitch_ = new ToggleSwitch(this);
    overlaySwitch_->setToolTip("Burn \"Original\" / \"Processed\" captions into the split output.");
    layout->addWidget(labeledRow("Burn in labels", overlaySwitch_, this));

    resolutionSeg_ = new SegmentedControl(this);
    // Segment order must match kDivisors below.
    resolutionSeg_->addSegment("1/1");
    resolutionSeg_->addSegment("1/2");
    resolutionSeg_->addSegment("1/4");
    resolutionSeg_->addSegment("1/8");
    resolutionSeg_->setCurrentIndex(divisorIndex(std::clamp(seed.preprocess.downscale, 1, 8)));
    resolutionSeg_->setToolTip("Process at a fraction of the source resolution (smaller = faster).");
    layout->addWidget(labeledRow("Resolution", resolutionSeg_, this));

    useRoiSwitch_ = new ToggleSwitch(this);
    useRoiSwitch_->setChecked(seed.preprocess.roiEnabled);
    useRoiSwitch_->setEnabled(seed.preprocess.roiEnabled);
    useRoiSwitch_->setToolTip(seed.preprocess.roiEnabled
                                  ? "Export only the selected region of interest; off exports the full frame."
                                  : "No region of interest is set — the full frame is exported.");
    layout->addWidget(labeledRow("Use current ROI", useRoiSwitch_, this));

    grayscaleSwitch_ = new ToggleSwitch(this);
    grayscaleSwitch_->setChecked(seed.grayscale);
    grayscaleSwitch_->setEnabled(seed.grayscaleAvailable);
    grayscaleSwitch_->setToolTip("Convert the incoming image to a single-channel grayscale texture.");
    layout->addWidget(labeledRow("Grayscale", grayscaleSwitch_, this));

    auto* magGroup = new QGroupBox("Magnification", this);
    auto* magLayout = new QVBoxLayout(magGroup);
    magControls_ = new MagnificationControls(magGroup);
    magLayout->addWidget(magControls_);
    layout->addWidget(magGroup);
    magControls_->setMaxLevels(seed.maxLevels); // cap before seeding
    magControls_->setParams(seed.magnification);
    magControls_->setGrayscale(seed.grayscale);

    fileFpsSpin_ = new QDoubleSpinBox(this);
    fileFpsSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    fileFpsSpin_->setRange(1.0, 240.0);
    fileFpsSpin_->setDecimals(3);
    fileFpsSpin_->setSingleStep(1.0);
    fileFpsSpin_->setSuffix(" fps");
    fileFpsSpin_->setKeyboardTracking(false);
    fileFpsSpin_->setValue(seed.fileFpsDefault > 0.0 ? std::min(seed.fileFpsDefault, 240.0) : 30.0);
    fileFpsSpin_->setFixedWidth(fieldW);
    fileFpsSpin_->setToolTip("Frame rate written to the output file (the playback speed of the result). "
                             "Independent of Capture FPS, so you can e.g. write a 1000 fps capture at 30 fps.");
    layout->addWidget(labeledRow("File FPS", fileFpsSpin_, this));

    // File sources only.
    if (!seed.isCamera && seed.frameCount > 0) {
        const int end = (seed.outFrame < 0 || seed.outFrame > seed.frameCount) ? seed.frameCount
                                                                               : seed.outFrame;
        startSpin_ = new QSpinBox(this);
        startSpin_->setRange(0, std::max(0, seed.frameCount - 1));
        startSpin_->setValue(std::clamp(seed.inFrame, 0, std::max(0, seed.frameCount - 1)));
        startSpin_->setToolTip("First frame to export (inclusive).");
        layout->addWidget(labeledRow("Start frame", startSpin_, this));

        endSpin_ = new QSpinBox(this);
        endSpin_->setRange(1, seed.frameCount);
        endSpin_->setValue(std::clamp(end, 1, seed.frameCount));
        endSpin_->setToolTip("End frame (exclusive).");
        layout->addWidget(labeledRow("End frame", endSpin_, this));
    }

    formatCombo_ = new QComboBox(this);
    // Item order must match ExportFormat; the index is cast to it directly.
    formatCombo_->addItem("MP4 (H.264)");
    formatCombo_->addItem("AVI (Motion JPEG)");
    formatCombo_->addItem("MKV (lossless)");
    formatCombo_->setToolTip("Output container + codec. MKV (FFV1) is lossless but produces very large files.");
    formatCombo_->setFixedWidth(fieldW);
    layout->addWidget(labeledRow("Format", formatCombo_, this));

    auto* outRow = new QWidget(this);
    auto* outLayout = new QHBoxLayout(outRow);
    outLayout->setContentsMargins(0, 0, 0, 0);
    pathEdit_ = new QLineEdit(outRow);
    pathEdit_->setReadOnly(true);
    pathEdit_->setPlaceholderText("Choose where to save…");
    auto* browseBtn = new QPushButton("Browse…", outRow);
    outLayout->addWidget(pathEdit_, 1);
    outLayout->addWidget(browseBtn);
    layout->addWidget(outRow);

    auto* buttons = new QDialogButtonBox(this);
    applyButton_ = buttons->addButton("Apply", QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(splitCombo_, &QComboBox::currentIndexChanged, this, [this](int) { syncOverlayEnabled(); });
    connect(formatCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (pathEdit_->text().isEmpty()) return;
        QFileInfo fi(pathEdit_->text());
        const QString ext = extensionFor(static_cast<ExportFormat>(formatCombo_->currentIndex()));
        pathEdit_->setText(fi.absolutePath() + "/" + fi.completeBaseName() + "." + ext);
    });
    connect(browseBtn, &QPushButton::clicked, this, &ExportSettingsDialog::browse);
    connect(grayscaleSwitch_, &ToggleSwitch::toggled, magControls_, &MagnificationControls::setGrayscale);
    connect(buttons, &QDialogButtonBox::accepted, this, &ExportSettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ExportSettingsDialog::reject);

    syncOverlayEnabled();
    updateApplyEnabled();

    resize(300, sizeHint().height());
}

void ExportSettingsDialog::syncOverlayEnabled() {
    const bool split = splitCombo_->currentIndex() != 0;
    overlaySwitch_->setEnabled(split);
    if (!split) overlaySwitch_->setChecked(false);
}

void ExportSettingsDialog::updateApplyEnabled() {
    applyButton_->setEnabled(!pathEdit_->text().isEmpty());
}

void ExportSettingsDialog::browse() {
    const auto fmt = static_cast<ExportFormat>(formatCombo_->currentIndex());
    const QString ext = extensionFor(fmt);
    const QString stem = seed_.suggestedBaseName.isEmpty() ? QStringLiteral("magnified")
                                                           : seed_.suggestedBaseName + "_magnified";
    QString filter;
    switch (fmt) {
    case ExportFormat::Mp4H264: filter = "MP4 video (*.mp4)"; break;
    case ExportFormat::AviMjpg: filter = "AVI video (*.avi)"; break;
    case ExportFormat::MkvFfv1: filter = "Matroska video (*.mkv)"; break;
    }
    QString start = pathEdit_->text();
    if (start.isEmpty()) start = stem + "." + ext;

    QString path = QFileDialog::getSaveFileName(this, "Save magnified video", start, filter);
    if (path.isEmpty()) return;
    QFileInfo fi(path);
    if (fi.suffix().compare(ext, Qt::CaseInsensitive) != 0)
        path = fi.absolutePath() + "/" + fi.completeBaseName() + "." + ext;
    pathEdit_->setText(path);
    updateApplyEnabled();
}

void ExportSettingsDialog::accept() {
    if (pathEdit_->text().isEmpty()) return;

    // Reject an empty or inverted range up front rather than failing late with no frames.
    if (startSpin_ && endSpin_ && startSpin_->value() >= endSpin_->value()) {
        QMessageBox::warning(this, "Invalid range", "Start frame must be less than end frame.");
        return;
    }

    // A camera export would otherwise record gigabytes into RAM and only fail once the writer
    // opens the file afterwards.
    const QFileInfo fi(pathEdit_->text());
    const QDir dir = fi.absoluteDir();
    if (!dir.exists() || !QFileInfo(dir.absolutePath()).isWritable()) {
        QMessageBox::warning(this, "Invalid output location",
                             QString("Cannot write to the folder:\n%1").arg(dir.absolutePath()));
        return;
    }
    if (fi.exists() &&
        QMessageBox::question(this, "Overwrite file?",
                              QString("%1 already exists. Overwrite it?").arg(fi.fileName()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    ExportRequest r;
    r.config.grayscale = grayscaleSwitch_->isChecked();
    r.config.preprocess = seed_.preprocess; // inherits the ROI rect; the fields below override
    r.config.preprocess.roiEnabled = useRoiSwitch_->isChecked();
    r.config.preprocess.downscale = kDivisors[std::clamp(resolutionSeg_->currentIndex(), 0, 3)];
    r.config.magnification = magControls_->collectParams();
    r.fileFps = fileFpsSpin_->value();
    r.split = static_cast<SplitMode>(splitCombo_->currentIndex());
    r.textOverlay = (r.split != SplitMode::None) && overlaySwitch_->isChecked();
    r.format = static_cast<ExportFormat>(formatCombo_->currentIndex());
    r.outputPath = pathEdit_->text().toStdString();
    if (startSpin_ && endSpin_) {
        r.startFrame = startSpin_->value();
        r.endFrame = endSpin_->value();
    }
    request_ = r;

    QDialog::accept();
}

} // namespace livim
