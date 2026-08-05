#pragma once

#include <QFont>
#include <QString>
#include <QWidget>

#include "core/Instrumentation.hpp" // StatsSnapshot
#include "ui/StatusHealth.hpp"      // stat -> health mapping

class QLabel;
class QDoubleSpinBox;

namespace livim {

// Bottom-of-window pipeline-health readout. Colour thresholds live in ui/StatusHealth.hpp.
class StatusStrip : public QWidget {
    Q_OBJECT
public:
    explicit StatusStrip(QWidget* parent = nullptr);

    // targetFps is the cadence a file is paced to; a camera ignores it and is coloured by its
    // dropped-frame share instead.
    void setStats(const StatsSnapshot& s, double targetFps, bool hasSource, bool cameraSource);

    // Silent: does not emit playbackFpsChanged. No-op for a non-positive rate.
    void setPlaybackFps(double fps);

    // Persistent right-aligned note (e.g. a software-decoding fallback warning). Empty text hides
    // it; it survives the next setStats() until cleared.
    void showNotice(const QString& text);
    void clearNotice() { showNotice(QString()); }

signals:
    void playbackFpsChanged(double fps);

private:
    using Health = statushealth::Health;

    struct Cell {
        QWidget* root = nullptr;
        QLabel*  dot = nullptr;
        QLabel*  value = nullptr;
    };

    void        setCell(Cell& c, Health h, const QString& value);
    static void applyState(QWidget* w, Health h); // sets the dynamic `state` property, then repolishes

    QFont captionFont_;
    QFont valueFont_;

    Cell            speed_;
    QDoubleSpinBox* playbackSpin_ = nullptr; // file only
    QLabel*         reported_ = nullptr;     // camera only
    QLabel*         slash_ = nullptr;

    QLabel* hint_ = nullptr; // hidden unless the pipeline is strained
    QLabel* notice_ = nullptr; // persistent fallback/status warning; hidden unless shown
};

} // namespace livim
