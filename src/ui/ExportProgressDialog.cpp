#include "ui/ExportProgressDialog.hpp"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace livim {
namespace {

QString formatElapsed(qint64 ms) {
    const qint64 totalSec = ms / 1000;
    return QString("%1:%2").arg(totalSec / 60, 2, 10, QChar('0')).arg(totalSec % 60, 2, 10, QChar('0'));
}

} // namespace

ExportProgressDialog::ExportProgressDialog(bool isCamera, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Export");
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    setMinimumWidth(340); // so growing readout text never resizes the window

    auto* layout = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);
    layout->addWidget(stack_);

    auto* recPage = new QWidget(stack_);
    auto* recLayout = new QVBoxLayout(recPage);
    recLabel_ = new QLabel("● REC", recPage);
    recLabel_->setStyleSheet("color: red; font-weight: bold; font-size: 16px;");
    recStats_ = new QLabel("0 frames", recPage);
    auto* recButtons = new QWidget(recPage);
    auto* recBtnLayout = new QHBoxLayout(recButtons);
    recBtnLayout->setContentsMargins(0, 0, 0, 0);
    auto* stopBtn = new QPushButton("Stop Recording", recButtons);
    auto* recAbortBtn = new QPushButton("Abort", recButtons);
    recBtnLayout->addWidget(stopBtn);
    recBtnLayout->addWidget(recAbortBtn);
    recLayout->addWidget(recLabel_);
    recLayout->addWidget(recStats_);
    recLayout->addWidget(recButtons);
    stack_->addWidget(recPage);

    auto* procPage = new QWidget(stack_);
    auto* procLayout = new QVBoxLayout(procPage);
    progressLabel_ = new QLabel("Processing…", procPage);
    progressBar_ = new QProgressBar(procPage);
    auto* procAbortBtn = new QPushButton("Abort", procPage);
    procLayout->addWidget(progressLabel_);
    procLayout->addWidget(progressBar_);
    procLayout->addWidget(procAbortBtn);
    stack_->addWidget(procPage);

    blinkTimer_ = new QTimer(this);
    blinkTimer_->setInterval(500);
    connect(blinkTimer_, &QTimer::timeout, this, [this] {
        blinkOn_ = !blinkOn_;
        // Blink by colour: hiding the label would collapse the layout.
        recLabel_->setStyleSheet(blinkOn_
                                     ? "color: red; font-weight: bold; font-size: 16px;"
                                     : "color: rgba(255,0,0,40); font-weight: bold; font-size: 16px;");
    });

    connect(stopBtn, &QPushButton::clicked, this, &ExportProgressDialog::stopRecordingRequested);
    connect(recAbortBtn, &QPushButton::clicked, this, &ExportProgressDialog::reject);
    connect(procAbortBtn, &QPushButton::clicked, this, &ExportProgressDialog::reject);

    if (isCamera) showRecordingPhase();
    else showProcessingPhase();
}

void ExportProgressDialog::showRecordingPhase() {
    stack_->setCurrentIndex(0);
    blinkOn_ = true;
    blinkTimer_->start();
}

void ExportProgressDialog::setRecordingStats(qint64 elapsedMs, qint64 frames, qint64 bytes) {
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    recStats_->setText(QString("%1   %2 frames   %3 MB")
                           .arg(formatElapsed(elapsedMs))
                           .arg(frames)
                           .arg(mb, 0, 'f', 1));
}

void ExportProgressDialog::showProcessingPhase() {
    blinkTimer_->stop();
    stack_->setCurrentIndex(1);
}

void ExportProgressDialog::setProgress(int framesDone, int framesTotal) {
    if (framesTotal > 0) {
        progressBar_->setRange(0, framesTotal);
        progressBar_->setValue(framesDone);
        const int pct = static_cast<int>(100.0 * framesDone / framesTotal);
        progressLabel_->setText(QString("Processing  %1 / %2  (%3%)")
                                    .arg(framesDone)
                                    .arg(framesTotal)
                                    .arg(pct));
    } else {
        progressBar_->setRange(0, 0); // indeterminate
        progressLabel_->setText(QString("Processing  %1 frames").arg(framesDone));
    }
}

void ExportProgressDialog::closeEvent(QCloseEvent* e) {
    if (!finished_ && !aborted_) {
        aborted_ = true;
        emit aborted();
    }
    QDialog::closeEvent(e);
}

void ExportProgressDialog::reject() {
    if (!finished_ && !aborted_) {
        aborted_ = true;
        emit aborted();
    }
    QDialog::reject();
}

} // namespace livim
