#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QSlider;

namespace livim {

// Horizontal [slider][numeric readout] presenting a continuous double value over an integer QSlider.
// valueChanged() fires only on user changes; programmatic setValue() is silent.
class SliderRow : public QWidget {
    Q_OBJECT
public:
    explicit SliderRow(QWidget* parent = nullptr);

    void   setRange(double min, double max);
    void   setSingleStep(double step);  // value granularity; the slider's integer tick size
    void   setDecimals(int decimals);
    void   setSuffix(const QString& suffix); // appended verbatim, e.g. " %" / " Hz"
    void   setValue(double v);          // silent
    void   setValueUser(double v);      // like setValue, but emits valueChanged on a real change
    double value() const { return value_; }

    void   setReadoutVisible(bool visible);
    QString text() const;               // value + suffix, as displayed

signals:
    void valueChanged(double v);
    void textChanged(const QString& text); // fires on user AND programmatic changes

private:
    void rebuildSlider();
    void onSliderMoved(int tick);
    void updateReadout();

    QSlider* slider_ = nullptr;
    QLabel*  readout_ = nullptr;

    double  min_ = 0.0;
    double  max_ = 100.0;
    double  step_ = 1.0;
    int     decimals_ = 0;
    QString suffix_;
    double  value_ = 0.0;
    bool    updating_ = false; // suppresses emit while syncing the slider programmatically
};

} // namespace livim
