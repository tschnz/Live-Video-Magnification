#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTimer;

namespace livim {

// Two-phase (Recording -> Processing) modal progress view driven by MainWindow; it owns no threads
// and never touches the Exporter. Closing the window counts as Abort.
class ExportProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportProgressDialog(bool isCamera, QWidget* parent = nullptr);

    void showRecordingPhase();
    void setRecordingStats(qint64 elapsedMs, qint64 frames, qint64 bytes);
    void showProcessingPhase();
    void setProgress(int framesDone, int framesTotal); // framesTotal < 0 => indeterminate

    // Call on Done/Error so a subsequent close() does not emit aborted().
    void markFinished() { finished_ = true; }

signals:
    void stopRecordingRequested();
    void aborted();

protected:
    void closeEvent(QCloseEvent* e) override;
    void reject() override;

private:
    QStackedWidget* stack_ = nullptr;

    QLabel*      recLabel_ = nullptr; // blinking "● REC"
    QLabel*      recStats_ = nullptr;
    QTimer*      blinkTimer_ = nullptr;
    bool         blinkOn_ = true;

    QProgressBar* progressBar_ = nullptr;
    QLabel*       progressLabel_ = nullptr;

    bool aborted_ = false;  // emit aborted() at most once
    bool finished_ = false;
};

} // namespace livim
