#include "ui/MainWindow.hpp"

#include <deque>
#include <memory>
#include <utility>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFont>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "export/BufferExportFrameSource.hpp"
#include "export/FileExportFrameSource.hpp"
#include "export/RecordingBuffer.hpp"
#include "ui/CameraControlsView.hpp"
#include "ui/CameraSelectDialog.hpp"
#include "ui/DisplayWidget.hpp"
#include "ui/ExportProgressDialog.hpp"
#include "ui/ExportSettingsDialog.hpp"
#include "ui/FileControlsView.hpp"
#include "ui/Icons.hpp"
#include "ui/ProcessingPanel.hpp"
#include "ui/StatusStrip.hpp"
#include "ui/Theme.hpp"
#include "ui/TimelineView.hpp"
#include "ui/ToggleSwitch.hpp"

#include <QColor>
#include <QPalette>

namespace livim {
namespace {
// Cap the in-RAM camera capture so a long recording can't OOM-kill the process. When hit,
// recording auto-stops cleanly and the frames captured so far are still exported.
constexpr std::size_t kRecordingCapBytes = 8ULL * 1024 * 1024 * 1024;
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("LiViM");

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* tb = new QToolBar("Main", this);
    tb->setObjectName("mainToolBar");
    tb->setMovable(false);
    tb->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, tb);
    topBar_ = tb;

    openFileBtn_ = new QPushButton("Open File", tb);
    openCamBtn_ = new QPushButton("Open Camera", tb);
    exportButton_ = new QPushButton("Export…", tb);
    exportButton_->setEnabled(false); // until a source is open
    exportButton_->setProperty("accent", true);
    exportButton_->setToolTip("Render the magnified video to a file with the chosen settings.");

    QFont toolbarCapFont = tb->font();
    toolbarCapFont.setPointSizeF(toolbarCapFont.pointSizeF() * 0.80);
    toolbarCapFont.setCapitalization(QFont::AllUppercase);
    toolbarCapFont.setLetterSpacing(QFont::PercentageSpacing, 106.0);
    toolbarCapFont.setWeight(QFont::DemiBold);

    auto* displayCaption = new QLabel("Display", tb);
    displayCaption->setObjectName("toolbarCaption");
    displayCaption->setFont(toolbarCapFont);

    auto* viewModeCombo = new QComboBox(tb);
    viewModeCombo->setFont(toolbarCapFont);
    // Item order must match DisplayWidget::ViewMode; the index is cast to it directly.
    viewModeCombo->addItem("Processed");
    viewModeCombo->addItem("Original");
    viewModeCombo->addItem("Side-by-Side");
    viewModeCombo->addItem("Top-and-Bottom");
    viewModeCombo->setToolTip(
        "Choose what the video area shows. Side-by-side / top-and-bottom show the original and the "
        "processed frame together, synced; ROI selection works in either pane.");

    inspectorToggleBtn_ = new QPushButton("Settings", tb);
    inspectorToggleBtn_->setCheckable(true);
    inspectorToggleBtn_->setChecked(true);
    inspectorToggleBtn_->setToolTip("Show or hide the settings panel.");

    fullscreenBtn_ = new QPushButton("Fullscreen", tb);
    fullscreenBtn_->setToolTip("Show the video fullscreen. Press Esc or F11 to exit.");

    // Glyph and tooltip are set in refreshToolbarIcons() so they track the active scheme.
    themeToggleBtn_ = new QPushButton(tb);

    for (QPushButton* b :
         {openFileBtn_, openCamBtn_, exportButton_, inspectorToggleBtn_, fullscreenBtn_, themeToggleBtn_})
        b->setIconSize(QSize(16, 16));

    // Grow the combo to the buttons' natural height (shrinking a button clips its descenders).
    viewModeCombo->setFixedHeight(openFileBtn_->sizeHint().height());

    loopControls_ = new QWidget(central);
    auto* loopLayout = new QHBoxLayout(loopControls_);
    loopLayout->setContentsMargins(0, 0, 0, 0);
    loopLayout->setSpacing(6);
    loopSwitch_ = new ToggleSwitch(loopControls_);
    loopSwitch_->setToolTip("Loop playback");
    loopLayout->addWidget(new QLabel("Loop", loopControls_));
    loopLayout->addWidget(loopSwitch_);
    loopControls_->setVisible(false); // shown only while a file is open

    tb->addWidget(openFileBtn_);
    tb->addWidget(openCamBtn_);
    tb->addWidget(exportButton_);
    tb->addSeparator();
    tb->addWidget(displayCaption);
    tb->addWidget(viewModeCombo);
    tb->addWidget(fullscreenBtn_);
    auto* tbSpacer = new QWidget(tb);
    tbSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(tbSpacer);
    tb->addWidget(inspectorToggleBtn_);
    tb->addWidget(themeToggleBtn_);

    display_ = new DisplayWidget(central);
    display_->setMinimumSize(640, 360);
    // The video owns keyboard focus: in fullscreen it is the visible widget that key events
    // (Esc / F11) bubble up from to MainWindow::keyPressEvent.
    display_->setFocusPolicy(Qt::StrongFocus);

    processingPanel_ = new ProcessingPanel(central);

    fileControls_ = new FileControlsView(central);
    cameraControls_ = new CameraControlsView(central);

    controlsStack_ = new QStackedWidget(central);
    controlsStack_->addWidget(new QWidget(central)); // index 0: blank (nothing open yet)
    controlsStack_->addWidget(fileControls_);
    controlsStack_->addWidget(cameraControls_);
    controlsStack_->setCurrentIndex(0);
    // Fixed width = the widest page, so Play/Pause keeps its position whichever view is shown.
    controlsStack_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    timeline_ = new TimelineView(central);
    timeline_->setVisible(false);

    // Stretchable container keeps the controls left-anchored when there is no timeline (camera).
    auto* timelineContainer = new QWidget(central);
    auto* timelineContainerLayout = new QHBoxLayout(timelineContainer);
    timelineContainerLayout->setContentsMargins(0, 0, 0, 0);
    timelineContainerLayout->addWidget(timeline_, 1);

    transportBar_ = new QWidget(central);
    auto* transportBar = transportBar_;
    auto* transportLayout = new QHBoxLayout(transportBar);
    transportLayout->setContentsMargins(6, 4, 6, 4);
    transportLayout->setSpacing(8);
    transportLayout->addWidget(controlsStack_);
    transportLayout->addWidget(timelineContainer, 1);
    transportLayout->addWidget(loopControls_);

    statusStrip_ = new StatusStrip(central);

    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setObjectName("mainSplitter");
    splitter->addWidget(display_);
    splitter->addWidget(processingPanel_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, true);
    splitter->setHandleWidth(1);
    splitter->setSizes({1000, 300});

    layout->addWidget(splitter, 1);
    layout->addWidget(transportBar_);
    layout->addWidget(statusStrip_);
    setCentralWidget(central);

    controller_.bindRenderer(display_);
    display_->setInstrumentation(controller_.instrumentation());

    connect(openFileBtn_, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(openCamBtn_, &QPushButton::clicked, this, &MainWindow::onOpenCamera);
    connect(exportButton_, &QPushButton::clicked, this, &MainWindow::onExport);
    connect(viewModeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const auto mode = static_cast<DisplayWidget::ViewMode>(index);
        display_->setViewMode(mode);
        // "Original" shows only the untouched frame, so skip the (heavy) magnification.
        controller_.setMagnificationActive(mode != DisplayWidget::ViewMode::Original);
    });
    connect(fullscreenBtn_, &QPushButton::clicked, this, [this] { setFullscreen(!isFullScreen()); });
    connect(inspectorToggleBtn_, &QPushButton::toggled, this,
            [this](bool on) { processingPanel_->setVisible(on); });
    connect(themeToggleBtn_, &QPushButton::clicked, this, [] {
        const auto next = theme::appliedScheme() == ColorScheme::Dark ? ColorScheme::Light
                                                                      : ColorScheme::Dark;
        // Re-applies palette + QSS; the PaletteChange runs changeEvent() -> refreshToolbarIcons().
        theme::overrideScheme(*static_cast<QApplication*>(QApplication::instance()), next);
    });

    refreshToolbarIcons();

    connect(statusStrip_, &StatusStrip::playbackFpsChanged, this, [this](double v) {
        controller_.setPlaybackFps(v);
        timeline_->setFps(v); // the timeline shows wall-clock time at the playback rate
    });

    connect(loopSwitch_, &ToggleSwitch::toggled, this, [this](bool on) { controller_.setLoop(on); });

    for (SourceControlsView* v : {static_cast<SourceControlsView*>(fileControls_),
                                  static_cast<SourceControlsView*>(cameraControls_)}) {
        connect(v, &SourceControlsView::playPauseRequested, this, &MainWindow::onPlayPause);
        connect(v, &SourceControlsView::stopRequested, this, &MainWindow::onStop);
    }

    connect(processingPanel_, &ProcessingPanel::grayscaleToggled, this,
            [this](bool on) { controller_.setGrayscale(on); });
    connect(processingPanel_, &ProcessingPanel::magnificationChanged, this,
            [this](MagnificationParams p) { controller_.setMagnification(p); });

    connect(processingPanel_, &ProcessingPanel::downscaleChanged, this,
            [this](int divisor) { controller_.setDownscale(divisor); });
    connect(processingPanel_, &ProcessingPanel::roiSelectModeChanged, this,
            [this](bool selecting) { display_->setRoiDrawingEnabled(selecting); });
    connect(processingPanel_, &ProcessingPanel::roiResetRequested, this, [this] { resetRoi(); });

    // ROI selection is sticky: it stays armed until Reset ROI, and each drawn rect is composed
    // onto the active ROI by setRoi().
    connect(display_, &DisplayWidget::roiSelected, this,
            [this](float x, float y, float w, float h) {
                controller_.setRoi(x, y, w, h);
                processingPanel_->setRoiActive(true);
            });

    connect(timeline_, &TimelineView::seekRequested, this,
            [this](std::int64_t frame) { controller_.seekFrame(frame); });
    connect(timeline_, &TimelineView::inOutChanged, this,
            [this](std::int64_t in, std::int64_t out) { controller_.setInOut(in, out); });

    // A scrub freezes playback while the handle is held; resume on release if it was playing.
    connect(timeline_, &TimelineView::scrubStarted, this, [this] {
        scrubActive_ = true;
        scrubResume_ = controller_.isPlaying(); // capture intent before pausing
        controller_.pause();
        updatePlayPauseButton();
    });
    connect(timeline_, &TimelineView::scrubFinished, this, [this] {
        scrubActive_ = false;
        if (scrubResume_) controller_.play();
        scrubResume_ = false;
        updatePlayPauseButton();
    });

    statsTimer_ = new QTimer(this);
    statsTimer_->setInterval(250);
    connect(statsTimer_, &QTimer::timeout, this, &MainWindow::refreshStats);
    statsTimer_->start();

    // Faster dedicated tick keeps the playhead smooth; the handle only auto-follows while playing.
    timelineTimer_ = new QTimer(this);
    timelineTimer_->setInterval(60);
    connect(timelineTimer_, &QTimer::timeout, this, [this] {
        if (!timeline_->isVisible()) return;
        timeline_->setFrameCount(controller_.frameCount());
        if (controller_.isPlaying())
            timeline_->setPlayheadFrame(controller_.currentFrame());
        else if (controller_.atEnd())
            timeline_->setPlayheadFrame(controller_.currentFrame());
    });
    timelineTimer_->start();

    // Started only while an export/record flow is active.
    exportTimer_ = new QTimer(this);
    exportTimer_->setInterval(100);
    connect(exportTimer_, &QTimer::timeout, this, &MainWindow::pollExport);
}

void MainWindow::showControls(SourceControlsView* view) {
    activeControls_ = view;
    controlsStack_->setCurrentWidget(view ? static_cast<QWidget*>(view)
                                          : controlsStack_->widget(0));
    const bool isFile = (view == static_cast<SourceControlsView*>(fileControls_));
    timeline_->setVisible(isFile);
    if (timeline_->isVisible()) {
        timeline_->setFps(controller_.playbackFps());
        timeline_->setFrameCount(controller_.frameCount());
        timeline_->setInOut(0, -1); // whole clip by default
        timeline_->setPlayheadFrame(controller_.currentFrame());
    }
    loopControls_->setVisible(isFile);
    if (isFile) syncFpsControls();

    // Channel count is fixed for a source's lifetime, so decide Grayscale availability once here.
    processingPanel_->setGrayscaleAvailable(controller_.sourceChannels() != 1);

    // Seeding Capture FPS also caps the Hz cutoffs at Nyquist and publishes the framerate.
    processingPanel_->setCaptureFps(controller_.reportedFps());
    processingPanel_->setMaxLevels(controller_.maxPyramidLevels());

    updatePlayPauseButton();
}

void MainWindow::syncFpsControls() {
    // setPlaybackFps blocks the input's signal so this seeding doesn't echo back into the setter.
    const double reported = controller_.reportedFps();
    if (reported > 0.0) {
        statusStrip_->setPlaybackFps(reported);
    } else {
        // No usable reported rate (e.g. a clip muxed as 0 fps). setPlaybackFps only updates the
        // widget, so drive the pipeline and timeline explicitly.
        constexpr double kDefaultPlaybackFps = 30.0;
        statusStrip_->setPlaybackFps(kDefaultPlaybackFps);
        controller_.setPlaybackFps(kDefaultPlaybackFps);
        timeline_->setFps(kDefaultPlaybackFps);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Closing mid-export would tear down the controller + exporter while the export worker thread
    // is still running -- a teardown-ordering hazard. Make the user finish or abort first.
    if (exportActive_) {
        QMessageBox::information(this, "Export in progress",
                                 "Please finish or abort the export before closing the window.");
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

// Only REQUESTS a window-state change; the chrome is reconciled in changeEvent once the window
// manager actually grants it (a Wayland fullscreen request can be refused).
void MainWindow::setFullscreen(bool on) {
    if (on) {
        if (exportActive_) return; // an export owns the UI lock
        if (isFullScreen()) return;
        wasMaximized_ = isMaximized();
        showFullScreen();
    } else {
        if (!isFullScreen()) return;
        if (wasMaximized_) showMaximized();
        else showNormal();
    }
}

void MainWindow::applyFullscreenUi(bool on) {
    if (fullscreen_ == on) return; // a WindowStateChange can fire more than once per toggle
    fullscreen_ = on;

    if (on) {
        // Disarm ROI drawing, since the panel hosting the toggle is about to be hidden; the active
        // ROI region stays. setRoiSelecting is signal-blocked, hence the explicit display call.
        processingPanel_->setRoiSelecting(false);
        display_->setRoiDrawingEnabled(false);
        display_->setFocus(); // key target for Esc / F11 once the chrome is hidden
    }

    // Keep the transport for a file so it stays scrubbable; hide everything else. Re-showing a
    // container never force-shows an explicitly-hidden child, so the camera-vs-file state
    // showControls() set on timeline_/loopControls_ survives untouched.
    const bool keepTransport = sourceOpen_ && sourceKind_ == SourceKind::File;
    topBar_->setVisible(!on);
    processingPanel_->setVisible(!on);
    statusStrip_->setVisible(!on);
    transportBar_->setVisible(!on || keepTransport);
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    // The window manager is authoritative for the actual fullscreen state; reconcile the chrome to
    // whatever state was really granted.
    if (event->type() == QEvent::WindowStateChange) applyFullscreenUi(isFullScreen());
    // The toolbar icons are palette-tinted pixmaps, so a live light/dark switch must repaint them.
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        refreshToolbarIcons();
}

void MainWindow::refreshToolbarIcons() {
    if (!themeToggleBtn_) return; // a PaletteChange can arrive before the toolbar is fully built
    const QColor txt = palette().color(QPalette::ButtonText);
    const QColor ink = palette().color(QPalette::HighlightedText); // ink on the accent Export button
    openFileBtn_->setIcon(icons::openFile(txt, 16));
    openCamBtn_->setIcon(icons::camera(txt, 16));
    inspectorToggleBtn_->setIcon(icons::sliders(txt, 16));
    fullscreenBtn_->setIcon(icons::fullscreen(txt, 16));
    exportButton_->setIcon(icons::exportArrow(ink, 16));

    // The appearance toggle shows the scheme it switches TO.
    const bool dark = theme::appliedScheme() == ColorScheme::Dark;
    themeToggleBtn_->setIcon(dark ? icons::sun(txt, 16) : icons::moon(txt, 16));
    themeToggleBtn_->setToolTip(dark ? "Switch to light appearance" : "Switch to dark appearance");
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    // Handled here (not an always-on QShortcut) so Escape only acts while fullscreen and otherwise
    // propagates normally to dialogs / spin boxes / combos.
    if (!exportActive_) {
        if (event->key() == Qt::Key_F11) {
            setFullscreen(!isFullScreen());
            return;
        }
        if (event->key() == Qt::Key_Escape && isFullScreen()) {
            setFullscreen(false);
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::resetRoi() {
    controller_.clearRoi();
    processingPanel_->setRoiActive(false);
    processingPanel_->setRoiSelecting(false);
    display_->setRoiDrawingEnabled(false);
}

void MainWindow::onOpenFile() {
    if (exportActive_) return;
    const QString path = QFileDialog::getOpenFileName(
        this, "Open video file", QString(),
        "Video files (*.mp4 *.mov *.avi *.mkv *.webm);;All files (*.*)");
    if (path.isEmpty()) return;

    if (!controller_.openFile(path.toStdString())) {
        QMessageBox::warning(this, "Open failed",
                             "Could not open the video file. Is the codec supported by ffmpeg?");
        return;
    }
    statusStrip_->showNotice(QString::fromStdString(controller_.lastOpenNotice()));
    currentFilePath_ = path;
    sourceKind_ = SourceKind::File;
    sourceOpen_ = true;
    resetRoi(); // a new source has a new frame geometry
    exportButton_->setEnabled(true);
    controller_.play();
    showControls(fileControls_);
}

void MainWindow::onOpenCamera() {
    if (exportActive_) return;
    CameraSelectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    const int index = dlg.selectedDeviceIndex();
    if (index < 0) return;

    if (!controller_.openCamera(index)) {
        QMessageBox::warning(this, "Open failed",
                             QString("Could not open camera \"%1\".").arg(dlg.selectedDeviceName()));
        return;
    }
    statusStrip_->clearNotice();
    currentFilePath_.clear();
    sourceKind_ = SourceKind::Camera;
    sourceOpen_ = true;
    resetRoi(); // a new source has a new frame geometry
    exportButton_->setEnabled(true);
    controller_.play();
    showControls(cameraControls_);
}

void MainWindow::onPlayPause() {
    if (controller_.isPlaying()) controller_.pause();
    else controller_.play();
    updatePlayPauseButton();
}

void MainWindow::onStop() {
    controller_.stop();
    timeline_->resetToStart();
    updatePlayPauseButton();
}

void MainWindow::updatePlayPauseButton() {
    if (!activeControls_) return;
    // While scrubbing, show the state we'll resume to so the button doesn't flicker mid-drag.
    activeControls_->setPlaying(scrubActive_ ? scrubResume_ : controller_.isPlaying());
}

void MainWindow::refreshStats() {
    const StatsSnapshot s = controller_.stats();
    // For a file the strip compares the achieved rate against this target; a camera ignores it and
    // colours FPS by dropped-frame share.
    const double targetFps = controller_.playbackFps();
    const bool cameraSource = sourceKind_ == SourceKind::Camera;
    statusStrip_->setStats(s, targetFps, sourceOpen_, cameraSource);

    // Reflect state changes that happen without a click (e.g. a file reaching its end).
    updatePlayPauseButton();
}

void MainWindow::onExport() {
    if (exportActive_ || !sourceOpen_) return;

    const bool isCamera = sourceKind_ == SourceKind::Camera;
    ExportSettingsDialog::Seed seed;
    seed.magnification = controller_.magnification(); // framerate = current Capture FPS
    seed.grayscale = controller_.grayscaleEnabled();
    seed.grayscaleAvailable = controller_.sourceChannels() != 1;
    seed.preprocess = controller_.preprocess();
    seed.maxLevels = controller_.maxPyramidLevels();
    seed.fileFpsDefault = controller_.playbackFps() > 0.0 ? controller_.playbackFps() : 30.0;
    seed.isCamera = isCamera;
    seed.frameCount = static_cast<int>(controller_.frameCount());
    seed.inFrame = static_cast<int>(timeline_->inFrame());
    seed.outFrame = static_cast<int>(timeline_->outFrame());
    seed.suggestedBaseName = isCamera ? QStringLiteral("camera")
                                      : QFileInfo(currentFilePath_).completeBaseName();

    ExportSettingsDialog dlg(seed, this);
    if (dlg.exec() != QDialog::Accepted) return;
    exportRequest_ = dlg.request();

    setExportUiActive(true);
    exportActive_ = true;

    exportProgress_ = new ExportProgressDialog(isCamera, this);
    connect(exportProgress_, &ExportProgressDialog::aborted, this, &MainWindow::onExportAborted);
    connect(exportProgress_, &ExportProgressDialog::stopRecordingRequested, this,
            &MainWindow::onStopRecording);

    if (isCamera) {
        // Record raw frames losslessly into a buffer; process them after Stop Recording.
        recordingPhase_ = true;
        recBuf_ = std::make_shared<RecordingBuffer>(kRecordingCapBytes);
        recElapsed_.start();
        controller_.beginCameraRecording(recBuf_);
    } else {
        // Halt live playback; the exporter opens its own capture.
        recordingPhase_ = false;
        controller_.pause();
        startFileProcessing();
    }
    exportProgress_->show();
    if (isCamera) {
        // Anchor the dialog near the window's bottom so the live raw feed stays visible.
        const QRect g = geometry();
        exportProgress_->move(g.center().x() - exportProgress_->width() / 2,
                              g.bottom() - exportProgress_->height() - 48);
    }
    exportTimer_->start();
}

void MainWindow::setExportUiActive(bool active) {
    if (active) exportResume_ = controller_.isPlaying();
    topBar_->setEnabled(!active);
    transportBar_->setEnabled(!active);
    processingPanel_->setEnabled(!active);
    if (!active && exportResume_) {
        controller_.play();
        exportResume_ = false;
    }
    updatePlayPauseButton();
}

void MainWindow::startFileProcessing() {
    // Publishing to the display mailbox previews the render live; the paused live source is not
    // touching the mailbox.
    exporter_.start(std::make_unique<FileExportFrameSource>(currentFilePath_.toStdString(),
                                                            exportRequest_.startFrame,
                                                            exportRequest_.endFrame),
                    exportRequest_, controller_.mailbox());
}

void MainWindow::onStopRecording() {
    if (!recordingPhase_) return;
    recordingPhase_ = false;
    controller_.endCameraRecording(); // must quiesce the camera before taking the frames

    std::deque<cv::Mat> frames = recBuf_->takeFrames();
    if (exportProgress_) exportProgress_->showProcessingPhase();
    exporter_.start(std::make_unique<BufferExportFrameSource>(std::move(frames)), exportRequest_,
                    controller_.mailbox());
}

void MainWindow::onExportAborted() {
    if (!exportActive_) return;
    exporter_.abort();
    if (recordingPhase_) controller_.endCameraRecording();
    exporter_.join(); // bounded: the worker checks abort once per frame
    finishExport();
}

void MainWindow::pollExport() {
    if (!exportActive_) return;

    if (recordingPhase_) {
        if (exportProgress_ && recBuf_)
            exportProgress_->setRecordingStats(recElapsed_.elapsed(),
                                               static_cast<qint64>(recBuf_->frameCount()),
                                               static_cast<qint64>(recBuf_->byteCount()));
        // The buffer hit its RAM cap and self-closed: finish via the same path as Stop Recording,
        // so the frames captured so far are still processed and written.
        if (recBuf_ && recBuf_->limitReached()) {
            const double capGb =
                static_cast<double>(recBuf_->capacityBytes()) / (1024.0 * 1024.0 * 1024.0);
            onStopRecording();
            QMessageBox::information(
                this, "Recording limit reached",
                QString("Recording stopped at the %1 GB memory limit. Processing the frames captured "
                        "so far.").arg(capGb, 0, 'f', 1));
        }
        return;
    }

    const ExportProgress p = exporter_.progress();
    switch (p.phase) {
    case ExportPhase::Processing:
    case ExportPhase::Finalizing:
        if (exportProgress_) exportProgress_->setProgress(p.framesDone, p.framesTotal);
        break;
    case ExportPhase::Done: {
        exporter_.join();
        const int frames = p.framesDone;
        const QString path = QString::fromStdString(p.outputPath.empty() ? exportRequest_.outputPath
                                                                          : p.outputPath);
        finishExport();
        QMessageBox::information(this, "Export complete",
                                 QString("Wrote %1 frames to\n%2").arg(frames).arg(path));
        break;
    }
    case ExportPhase::Error: {
        exporter_.join();
        const QString msg = QString::fromStdString(p.error);
        finishExport();
        QMessageBox::warning(this, "Export failed", msg);
        break;
    }
    case ExportPhase::Aborted:
        exporter_.join();
        finishExport();
        break;
    case ExportPhase::Idle:
        break;
    }
}

void MainWindow::finishExport() {
    exportTimer_->stop();
    if (exportProgress_) {
        exportProgress_->markFinished(); // so close() doesn't read as an abort
        exportProgress_->close();
        exportProgress_->deleteLater();
        exportProgress_ = nullptr;
    }
    recBuf_.reset();
    recordingPhase_ = false;
    exportActive_ = false;
    setExportUiActive(false);
}

} // namespace livim
