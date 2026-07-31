#include "ui/ToggleSwitch.hpp"

#include <algorithm>

#include <QEasingCurve>
#include <QEnterEvent>
#include <QPainter>
#include <QPropertyAnimation>

#include "ui/Theme.hpp"

namespace livim {
namespace {

constexpr int   kTrackW = 40;
constexpr int   kTrackH = 22;
constexpr qreal kKnobInset = 2.0; // gap between knob and track edge, px
constexpr int   kAnimMs = 140;

using theme::mix;

} // namespace

ToggleSwitch::ToggleSwitch(QWidget* parent) : QAbstractButton(parent) {
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    anim_ = new QPropertyAnimation(this, "position", this);
    anim_->setDuration(kAnimMs);
    anim_->setEasingCurve(QEasingCurve::InOutCubic);

    connect(this, &QAbstractButton::toggled, this, [this](bool on) { animateTo(on); });
}

QSize ToggleSwitch::sizeHint() const { return QSize(kTrackW + 6, kTrackH + 6); } // +3 px/side for the focus ring

void ToggleSwitch::setPosition(qreal p) {
    p = std::clamp(p, 0.0, 1.0);
    if (qFuzzyCompare(pos_ + 1.0, p + 1.0)) return;
    pos_ = p;
    update();
}

void ToggleSwitch::animateTo(bool on) {
    const qreal target = on ? 1.0 : 0.0;
    if (qFuzzyCompare(pos_ + 1.0, target + 1.0)) return;
    anim_->stop();
    anim_->setStartValue(pos_);
    anim_->setEndValue(target);
    anim_->start();
}

void ToggleSwitch::enterEvent(QEnterEvent* e) {
    hovered_ = true;
    update();
    QAbstractButton::enterEvent(e);
}

void ToggleSwitch::leaveEvent(QEvent* e) {
    hovered_ = false;
    update();
    QAbstractButton::leaveEvent(e);
}

void ToggleSwitch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPalette& pal = palette();
    const bool enabled = isEnabled();

    const qreal w = static_cast<qreal>(kTrackW);
    const qreal h = static_cast<qreal>(kTrackH);
    const qreal x = (static_cast<qreal>(width()) - w) / 2.0;
    const qreal y = (static_cast<qreal>(height()) - h) / 2.0;
    const QRectF track(x, y, w, h);
    const qreal radius = h / 2.0;

    // Track fill cross-fades Mid (off) -> Highlight (on) with the knob slide.
    const QColor offFill = pal.color(QPalette::Mid);
    const QColor onFill = pal.color(QPalette::Highlight);
    QColor trackFill = mix(offFill, onFill, pos_);
    if (!enabled) trackFill = mix(trackFill, pal.color(QPalette::Window), 0.5);

    p.setPen(Qt::NoPen);
    p.setBrush(trackFill);
    p.drawRoundedRect(track, radius, radius);

    // Border fades out as the accent fill takes over.
    QColor border = pal.color(QPalette::Mid);
    border.setAlphaF(static_cast<float>(1.0 - pos_));
    if (border.alpha() > 0) {
        p.setPen(QPen(border, 1.0));
        p.setBrush(Qt::NoBrush);
        const QRectF inner = track.adjusted(0.5, 0.5, -0.5, -0.5);
        p.drawRoundedRect(inner, radius - 0.5, radius - 0.5);
    }

    const qreal knobD = h - 2.0 * kKnobInset;
    const qreal travel = w - 2.0 * kKnobInset - knobD;
    const qreal knobX = x + kKnobInset + pos_ * travel;
    const QRectF knob(knobX, y + kKnobInset, knobD, knobD);

    QColor knobColor(255, 255, 255);
    if (!enabled) knobColor = mix(knobColor, pal.color(QPalette::Window), 0.4);

    p.setPen(QPen(QColor(0, 0, 0, 38), 1.0));
    p.setBrush(knobColor);
    p.drawEllipse(knob);

    if (hasFocus()) {
        QColor ring = pal.color(QPalette::Highlight);
        ring.setAlphaF(0.9f);
        p.setPen(QPen(ring, 2.0));
        p.setBrush(Qt::NoBrush);
        const QRectF ringRect = track.adjusted(-2.0, -2.0, 2.0, 2.0);
        p.drawRoundedRect(ringRect, radius + 2.0, radius + 2.0);
    } else if (hovered_ && enabled) {
        QColor ring = pal.color(QPalette::Highlight);
        ring.setAlphaF(0.35f);
        p.setPen(QPen(ring, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(-1.0, -1.0, 1.0, 1.0), radius + 1.0, radius + 1.0);
    }
}

} // namespace livim
