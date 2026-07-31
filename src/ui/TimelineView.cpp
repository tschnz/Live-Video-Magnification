#include "ui/TimelineView.hpp"

#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>

#include "ui/Theme.hpp"

namespace livim {
namespace {

constexpr int kTimeTextW = 120; // reserved width for the "cur / total" label, px
constexpr int kMargin = 10;
constexpr int kHitPx = 9;       // grab threshold for the in/out handles, px

QString formatTime(double seconds) {
    if (seconds < 0.0 || !std::isfinite(seconds)) seconds = 0.0;
    const int totalSec = static_cast<int>(seconds);
    const int h = totalSec / 3600;
    const int m = (totalSec % 3600) / 60;
    const int s = totalSec % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

} // namespace

TimelineView::TimelineView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(false);
    setMinimumHeight(34);
}

QSize TimelineView::sizeHint() const { return QSize(400, 34); }

double TimelineView::trackLeft() const { return kMargin; }
double TimelineView::trackRight() const { return width() - kTimeTextW - kMargin; }

int TimelineView::frameToX(std::int64_t f) const {
    const double denom = static_cast<double>(std::max<std::int64_t>(1, total_));
    const double frac = std::clamp(static_cast<double>(f) / denom, 0.0, 1.0);
    return static_cast<int>(std::lround(trackLeft() + frac * (trackRight() - trackLeft())));
}

std::int64_t TimelineView::xToFrame(int x) const {
    const double span = trackRight() - trackLeft();
    if (span <= 0.0) return 0;
    const double frac = std::clamp((x - trackLeft()) / span, 0.0, 1.0);
    return static_cast<std::int64_t>(std::llround(frac * static_cast<double>(total_)));
}

// Confine a playhead frame to [in_, out_ - 1]; out_ is exclusive.
std::int64_t TimelineView::clampToRange(std::int64_t f) const {
    if (total_ <= 0) return 0;
    return std::clamp<std::int64_t>(f, in_, std::max<std::int64_t>(in_, out_ - 1));
}

QString TimelineView::timeText() const {
    const double fps = fps_ > 0.0 ? fps_ : 30.0;
    return formatTime(static_cast<double>(playhead_) / fps) + " / " + formatTime(static_cast<double>(total_) / fps);
}

void TimelineView::setFrameCount(std::int64_t total) {
    total_ = std::max<std::int64_t>(0, total);
    if (out_ <= 0 || out_ > total_) out_ = total_;
    in_ = std::clamp<std::int64_t>(in_, 0, std::max<std::int64_t>(0, out_ - 1));
    playhead_ = clampToRange(playhead_);
    update();
}

void TimelineView::setPlayheadFrame(std::int64_t frame) {
    if (drag_ == Drag::Playhead) return;
    playhead_ = std::clamp<std::int64_t>(frame, 0, std::max<std::int64_t>(0, total_));
    update();
}

void TimelineView::setFps(double fps) {
    fps_ = fps;
    update();
}

void TimelineView::setInOut(std::int64_t in, std::int64_t out) {
    if (total_ <= 0) { in_ = 0; out_ = 0; update(); return; }
    out_ = out < 0 ? total_ : std::clamp<std::int64_t>(out, 1, total_);
    in_ = std::clamp<std::int64_t>(in, 0, out_ - 1);
    playhead_ = clampToRange(playhead_);
    update();
}

void TimelineView::resetToStart() {
    if (drag_ == Drag::Playhead) return;
    playhead_ = in_; // "start" is the in-point, not frame 0
    update();
}

void TimelineView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const double left = trackLeft(), right = trackRight();
    const int cy = height() / 2;
    const bool active = total_ > 0;

    // accent2 isn't a standard QPalette role, so it comes from the design tokens directly.
    const QPalette& pal = palette();
    const bool dark = pal.color(QPalette::Window).lightnessF() < 0.5;
    const QColor range = theme::palette(dark ? ColorScheme::Dark : ColorScheme::Light).accent2;

    QRectF groove(left, cy - 2, std::max(0.0, right - left), 4);
    p.setPen(Qt::NoPen);
    p.setBrush(pal.color(QPalette::Mid));
    p.drawRoundedRect(groove, 2, 2);

    if (active) {
        const int inX = frameToX(in_), outX = frameToX(out_);
        p.setBrush(range);
        p.drawRoundedRect(QRectF(inX, cy - 2, std::max(0, outX - inX), 4), 2, 2);

        p.drawRect(QRectF(inX - 2, cy - 9, 4, 18));
        p.drawRect(QRectF(outX - 2, cy - 9, 4, 18));

        const int px = frameToX(playhead_);
        p.setBrush(pal.color(QPalette::Text));
        p.drawRect(QRectF(px - 1, cy - 11, 2, 22));
        p.drawEllipse(QPointF(px, cy), 5, 5);
    }

    QColor timeColor = pal.color(QPalette::WindowText);
    timeColor.setAlphaF(0.7f);
    p.setPen(timeColor);
    p.drawText(QRect(static_cast<int>(right) + kMargin, 0, kTimeTextW - kMargin, height()),
               Qt::AlignVCenter | Qt::AlignRight, timeText());
}

void TimelineView::mousePressEvent(QMouseEvent* e) {
    if (total_ <= 0 || e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    const int x = static_cast<int>(e->position().x());
    const int inX = frameToX(in_), outX = frameToX(out_);
    if (std::abs(x - inX) <= kHitPx) {
        drag_ = Drag::In;
    } else if (std::abs(x - outX) <= kHitPx) {
        drag_ = Drag::Out;
    } else {
        drag_ = Drag::Playhead;
        playhead_ = clampToRange(xToFrame(x));
        emit scrubStarted();
        emit seekRequested(playhead_);
    }
    update();
}

void TimelineView::mouseMoveEvent(QMouseEvent* e) {
    if (drag_ == Drag::None) return;
    const int x = static_cast<int>(e->position().x());
    switch (drag_) {
    case Drag::Playhead:
        playhead_ = clampToRange(xToFrame(x));
        emit seekRequested(playhead_);
        break;
    case Drag::In:
        in_ = std::clamp<std::int64_t>(xToFrame(x), 0, std::max<std::int64_t>(0, out_ - 1));
        emit inOutChanged(in_, out_);
        break;
    case Drag::Out:
        out_ = std::clamp<std::int64_t>(xToFrame(x), in_ + 1, total_);
        emit inOutChanged(in_, out_);
        break;
    case Drag::None:
        break;
    }
    update();
}

void TimelineView::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    const Drag was = drag_;
    drag_ = Drag::None;
    if (was == Drag::Playhead) {
        emit scrubFinished();
    } else if (was == Drag::In || was == Drag::Out) {
        // The moved range may have left the playhead outside it.
        const std::int64_t clamped = clampToRange(playhead_);
        if (clamped != playhead_) {
            playhead_ = clamped;
            emit seekRequested(playhead_);
            update();
        }
    }
}

} // namespace livim
