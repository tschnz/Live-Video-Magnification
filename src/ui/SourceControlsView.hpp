#pragma once

#include <QWidget>

class QEvent;
class QPushButton;

namespace livim {

// Base class for a source-specific control panel with shared, source-agnostic transport signals.
class SourceControlsView : public QWidget {
    Q_OBJECT
public:
    explicit SourceControlsView(QWidget* parent = nullptr) : QWidget(parent) {}
    ~SourceControlsView() override = default;

    virtual void setPlaying(bool playing) = 0;

protected:
    static constexpr int kTransportButtonW = 34;
    static constexpr int kTransportButtonH = 34;

    QPushButton* makeTransportButton();

    // Called on construction and on theme changes.
    virtual void refreshIcons() {}

    void changeEvent(QEvent* event) override;

signals:
    void playPauseRequested();
    void stopRequested();          // not emitted by views without a Stop control
    void loopToggled(bool enabled); // not emitted by views without a Loop control
};

} // namespace livim
