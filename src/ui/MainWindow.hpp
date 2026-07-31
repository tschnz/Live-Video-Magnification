#pragma once

#include <memory>

#include <QElapsedTimer>
#include <QMainWindow>
#include <QString>

#include "export/Exporter.hpp"
#include "pipeline/PlaybackController.hpp"
#include "source/ISource.hpp" // SourceKind

class QEvent;
class QKeyEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;

namespace livim {

class DisplayWidget;
class ToggleSwitch;
class SourceControlsView;
class FileControlsView;
class CameraControlsView;
class TimelineView;
class ProcessingPanel;
class StatusStrip;
class ExportProgressDialog;
class RecordingBuffer;

// The application window. Controls invoke PlaybackController via signals/slots; a QTimer polls
// stats. No pixel data ever travels through a signal/slot.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onOpenFile();
    void onOpenCamera();
    void onPlayPause();
    void onStop();
    void refreshStats();
    void onExport();

private:
    void showControls(SourceControlsView* view);
    void resetRoi();
    void updatePlayPauseButton();
    void syncFpsControls();
    void refreshToolbarIcons();

    void setFullscreen(bool on);      // requests the window-state change only
    void applyFullscreenUi(bool on);  // hides/shows chrome to match the state actually granted

    void setExportUiActive(bool active); // disables the rest of the UI; captures/restores play state
    void startFileProcessing();
    void onStopRecording();  // camera: end recording, process the captured buffer
    void onExportAborted();
    void pollExport();       // exportTimer_ tick
    void finishExport();

    PlaybackController controller_;
    DisplayWidget* display_ = nullptr;
    TimelineView* timeline_ = nullptr;
    ProcessingPanel* processingPanel_ = nullptr;

    QStackedWidget* controlsStack_ = nullptr;
    FileControlsView* fileControls_ = nullptr;
    CameraControlsView* cameraControls_ = nullptr;
    SourceControlsView* activeControls_ = nullptr;

    QWidget*      loopControls_ = nullptr; // file only
    ToggleSwitch* loopSwitch_ = nullptr;

    StatusStrip* statusStrip_ = nullptr;
    QTimer* statsTimer_ = nullptr;
    QTimer* timelineTimer_ = nullptr;

    bool scrubActive_ = false;  // true while the user is dragging the timeline handle
    bool scrubResume_ = false;  // playback was running when the scrub began, so resume on drop

    bool fullscreen_ = false;   // last chrome state applied (idempotency guard)
    bool wasMaximized_ = false; // restore this on leaving fullscreen

    QWidget*     topBar_ = nullptr;
    QWidget*     transportBar_ = nullptr;

    // Buttons are members so their icons can be re-tinted on a theme change.
    QPushButton* openFileBtn_ = nullptr;
    QPushButton* openCamBtn_ = nullptr;
    QPushButton* fullscreenBtn_ = nullptr;
    QPushButton* themeToggleBtn_ = nullptr;
    QPushButton* inspectorToggleBtn_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    bool         sourceOpen_ = false;
    SourceKind   sourceKind_ = SourceKind::File;
    QString      currentFilePath_;         // so the exporter can re-decode the file

    Exporter                         exporter_;
    std::shared_ptr<RecordingBuffer> recBuf_;
    ExportProgressDialog*            exportProgress_ = nullptr;
    QTimer*                          exportTimer_ = nullptr;
    QElapsedTimer                    recElapsed_;
    ExportRequest                    exportRequest_; // captured at Apply, used through the whole flow
    bool                             exportActive_ = false;
    bool                             exportResume_ = false;   // resume playback when the flow ends
    bool                             recordingPhase_ = false; // camera: recording, not yet processing
};

} // namespace livim
