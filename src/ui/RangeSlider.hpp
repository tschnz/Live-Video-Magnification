#pragma once

#include <QWidget>

namespace livim {

// Two-handle slider bounding a [low, high] range; values snap to a step and handles cannot cross.
// valuesChanged() fires only on user changes; programmatic setValues() is silent.
class RangeSlider : public QWidget {
    Q_OBJECT
public:
    explicit RangeSlider(QWidget* parent = nullptr);

    void   setRange(double min, double max); // clamps the current values into the new range
    void   setStep(double step);             // value granularity the handles snap to (>0)
    void   setLogScale(bool on);             // log pixel mapping only (needs min > 0); value/step stay linear
    void   setValues(double low, double high); // silent; clamps to range and keeps low <= high
    double lowValue() const { return low_; }
    double highValue() const { return high_; }

    QSize sizeHint() const override;

signals:
    void valuesChanged(double low, double high);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    enum class Handle { None, Low, High };

    double trackLeft() const;
    double trackRight() const;
    int    valueToX(double v) const;
    double xToValue(int x) const;
    double snap(double v) const;
    void   enforceGap();                // keep low_ < high_ by at least one step
    Handle nearestHandle(int x) const;
    void   moveActiveTo(int x);
    void   nudgeActive(double delta);

    bool   logScale() const { return log_ && min_ > 0.0 && max_ > min_; }

    double min_  = 0.0;
    double max_  = 100.0;
    double step_ = 1.0;
    double low_  = 0.0;
    double high_ = 100.0;
    bool   log_  = false; // logarithmic pixel axis when set and min_ > 0
    Handle drag_   = Handle::None;
    Handle hover_  = Handle::None;
    Handle active_ = Handle::High; // the handle keyboard arrows adjust
};

} // namespace livim
