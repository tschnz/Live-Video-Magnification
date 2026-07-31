#include "ui/RangeSlider.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib> // std::abs(int)

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

namespace livim {
namespace {

constexpr double kTrackH = 5.0;   // groove thickness, px
constexpr double kHandleR = 7.0;  // handle radius, px
constexpr int    kHitPx = 10;     // grab radius around a handle, px

} // namespace

RangeSlider::RangeSlider(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(static_cast<int>(2 * kHandleR + 6));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(120);
}

QSize RangeSlider::sizeHint() const { return QSize(200, static_cast<int>(2 * kHandleR + 6)); }

// Handle-centre travel range, inset by the handle radius so extremes never clip.
double RangeSlider::trackLeft() const { return kHandleR; }
double RangeSlider::trackRight() const { return width() - kHandleR; }

int RangeSlider::valueToX(double v) const {
    double frac;
    if (logScale()) {
        frac = std::log(std::clamp(v, min_, max_) / min_) / std::log(max_ / min_);
    } else {
        const double span = max_ - min_;
        frac = span > 0.0 ? std::clamp((v - min_) / span, 0.0, 1.0) : 0.0;
    }
    return static_cast<int>(std::lround(trackLeft() + frac * (trackRight() - trackLeft())));
}

double RangeSlider::xToValue(int x) const {
    const double span = trackRight() - trackLeft();
    if (span <= 0.0) return min_;
    const double frac = std::clamp((static_cast<double>(x) - trackLeft()) / span, 0.0, 1.0);
    if (logScale()) return min_ * std::pow(max_ / min_, frac);
    return min_ + frac * (max_ - min_);
}

double RangeSlider::snap(double v) const {
    const double s = step_ > 0.0 ? step_ : 1.0;
    return std::clamp(min_ + std::round((v - min_) / s) * s, min_, max_);
}

// Keep low_ < high_ by at least one step; prefer pushing high up, else pull low down.
void RangeSlider::enforceGap() {
    const double s = step_ > 0.0 ? step_ : 1.0;
    if (high_ - low_ >= s) return;
    if (low_ + s <= max_) high_ = std::min(max_, low_ + s);
    else                  low_ = std::max(min_, high_ - s);
}

void RangeSlider::setRange(double min, double max) {
    min_ = min;
    max_ = max < min ? min : max;
    low_ = snap(low_);
    high_ = snap(high_);
    enforceGap();
    update();
}

void RangeSlider::setStep(double step) {
    step_ = step > 0.0 ? step : 1.0;
    low_ = snap(low_);
    high_ = snap(high_);
    enforceGap();
    update();
}

void RangeSlider::setLogScale(bool on) {
    if (log_ == on) return;
    log_ = on;
    update();
}

void RangeSlider::setValues(double low, double high) {
    low_ = snap(low);
    high_ = snap(high);
    if (high_ < low_) high_ = low_; // tolerate inverted input
    enforceGap();
    update();
}

RangeSlider::Handle RangeSlider::nearestHandle(int x) const {
    const int lx = valueToX(low_), hx = valueToX(high_);
    const int dl = std::abs(x - lx), dh = std::abs(x - hx);
    if (dl < dh) return Handle::Low;
    if (dh < dl) return Handle::High;
    return x < lx ? Handle::Low : Handle::High;
}

void RangeSlider::moveActiveTo(int x) {
    const double s = step_ > 0.0 ? step_ : 1.0;
    double v = snap(xToValue(x));
    if (active_ == Handle::Low)
        v = std::min(v, high_ - s);
    else
        v = std::max(v, low_ + s);
    v = std::clamp(v, min_, max_);
    double& target = (active_ == Handle::Low) ? low_ : high_;
    if (v != target) {
        target = v;
        update();
        emit valuesChanged(low_, high_);
    }
}

void RangeSlider::nudgeActive(double delta) {
    const double s = step_ > 0.0 ? step_ : 1.0;
    if (active_ == Handle::Low) {
        const double v = std::clamp(snap(low_ + delta), min_, std::max(min_, high_ - s));
        if (v != low_) { low_ = v; update(); emit valuesChanged(low_, high_); }
    } else {
        const double v = std::clamp(snap(high_ + delta), std::min(max_, low_ + s), max_);
        if (v != high_) { high_ = v; update(); emit valuesChanged(low_, high_); }
    }
}

void RangeSlider::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const double cy = height() / 2.0;
    const QPalette& pal = palette();
    const QColor groove = pal.color(QPalette::Mid);       // theme @line
    const QColor fill = pal.color(QPalette::Highlight);   // theme @accent
    const QColor border = pal.color(QPalette::Mid);

    p.setPen(Qt::NoPen);
    p.setBrush(groove);
    p.drawRoundedRect(QRectF(0.0, cy - kTrackH / 2.0, static_cast<double>(width()), kTrackH),
                      kTrackH / 2.0, kTrackH / 2.0);

    const int lx = valueToX(low_), hx = valueToX(high_);
    p.setBrush(fill);
    p.drawRoundedRect(QRectF(lx, cy - kTrackH / 2.0, std::max(0, hx - lx), kTrackH),
                      kTrackH / 2.0, kTrackH / 2.0);

    auto drawHandle = [&](int x, bool lit) {
        p.setBrush(QColor("#FFFFFF"));
        p.setPen(QPen(lit ? fill : border, 1.0));
        p.drawEllipse(QPointF(static_cast<double>(x), cy), kHandleR, kHandleR);
    };
    const bool lowLit = drag_ == Handle::Low || (drag_ == Handle::None && hover_ == Handle::Low);
    const bool highLit = drag_ == Handle::High || (drag_ == Handle::None && hover_ == Handle::High);
    // Inactive handle first so the active one wins any overlap.
    if (active_ == Handle::Low) { drawHandle(hx, highLit); drawHandle(lx, lowLit); }
    else                        { drawHandle(lx, lowLit);  drawHandle(hx, highLit); }
}

void RangeSlider::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }
    const int x = static_cast<int>(e->position().x());
    drag_ = nearestHandle(x);
    active_ = drag_;
    moveActiveTo(x);
    update();
}

void RangeSlider::mouseMoveEvent(QMouseEvent* e) {
    const int x = static_cast<int>(e->position().x());
    if (drag_ != Handle::None) {
        moveActiveTo(x);
        return;
    }
    // `cand`, not `near`: `near`/`far` are <windows.h> macros.
    const Handle cand = nearestHandle(x);
    const int hx = valueToX(cand == Handle::Low ? low_ : high_);
    const Handle h = std::abs(x - hx) <= kHitPx ? cand : Handle::None;
    if (h != hover_) { hover_ = h; update(); }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) { QWidget::mouseReleaseEvent(e); return; }
    drag_ = Handle::None;
    update();
}

void RangeSlider::leaveEvent(QEvent*) {
    if (hover_ != Handle::None) { hover_ = Handle::None; update(); }
}

void RangeSlider::keyPressEvent(QKeyEvent* e) {
    const double s = step_ > 0.0 ? step_ : 1.0;
    switch (e->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:     nudgeActive(-s); break;
    case Qt::Key_Right:
    case Qt::Key_Up:       nudgeActive(+s); break;
    case Qt::Key_PageDown: nudgeActive(-10.0 * s); break;
    case Qt::Key_PageUp:   nudgeActive(+10.0 * s); break;
    case Qt::Key_Home:
        active_ = Handle::Low;
        update();
        break;
    case Qt::Key_End:
        active_ = Handle::High;
        update();
        break;
    default:
        QWidget::keyPressEvent(e);
        return;
    }
}

} // namespace livim
