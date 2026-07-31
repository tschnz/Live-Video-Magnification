#include "ui/SegmentedControl.hpp"

#include <algorithm>

#include <QEvent>
#include <QFontMetrics>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include "ui/Theme.hpp"

namespace livim {
namespace {

constexpr int   kPadX = 14;      // horizontal text padding, px
constexpr int   kHeight = 28;    // track height, px
constexpr qreal kBorder = 1.0;   // border width, px
constexpr qreal kSelInset = 2.0; // selected fill inset from track edge, px

using theme::mix;

} // namespace

SegmentedControl::SegmentedControl(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
}

void SegmentedControl::addSegment(const QString& text) {
    segments_.append(text);
    if (current_ < 0) current_ = 0; // first segment auto-selects silently
    updateGeometry();
    update();
}

void SegmentedControl::setCurrentIndex(int index) {
    if (segments_.isEmpty()) return;
    index = std::clamp(index, 0, count() - 1);
    if (index == current_) return;
    current_ = index;
    update();
    emit currentIndexChanged(current_);
}

QSize SegmentedControl::sizeHint() const {
    const QFontMetrics fm(font());
    int segW = 1;
    for (const QString& s : segments_)
        segW = std::max(segW, fm.horizontalAdvance(s));
    const int per = segW + 2 * kPadX;
    const int n = std::max(1, count());
    return QSize(per * n, kHeight);
}

QRectF SegmentedControl::segmentRect(int index) const {
    const int n = count();
    if (n <= 0 || index < 0 || index >= n) return {};
    const qreal w = static_cast<qreal>(width());
    const qreal cellW = w / static_cast<qreal>(n);
    // Round boundaries to whole pixels so adjacent cells abut exactly.
    const qreal x0 = std::round(static_cast<qreal>(index) * cellW);
    const qreal x1 = std::round(static_cast<qreal>(index + 1) * cellW);
    return QRectF(x0, 0.0, x1 - x0, static_cast<qreal>(height()));
}

int SegmentedControl::segmentAt(const QPoint& pt) const {
    for (int i = 0; i < count(); ++i)
        if (segmentRect(i).contains(QPointF(pt)))
            return i;
    return -1;
}

void SegmentedControl::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPalette& pal = palette();
    const bool enabled = isEnabled();
    const qreal radius = static_cast<qreal>(metrics::radius);

    const QRectF track = QRectF(rect()).adjusted(kBorder / 2.0, kBorder / 2.0, -kBorder / 2.0, -kBorder / 2.0);

    QColor fieldFill = pal.color(QPalette::Base);
    if (!enabled) fieldFill = mix(fieldFill, pal.color(QPalette::Window), 0.5);
    p.setPen(QPen(pal.color(QPalette::Mid), kBorder));
    p.setBrush(fieldFill);
    p.drawRoundedRect(track, radius, radius);

    if (count() == 0) return;

    if (current_ >= 0) {
        QRectF sel = segmentRect(current_).adjusted(kSelInset, kSelInset, -kSelInset, -kSelInset);
        QColor accent = pal.color(QPalette::Highlight);
        if (!enabled) accent = mix(accent, pal.color(QPalette::Window), 0.45);
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        const qreal selR = std::max(0.0, radius - kSelInset);
        p.drawRoundedRect(sel, selR, selR);
    }

    p.setFont(font());
    for (int i = 0; i < count(); ++i) {
        const QRectF cell = segmentRect(i);
        const bool selected = (i == current_);
        QColor textColor;
        if (selected) {
            textColor = pal.color(QPalette::HighlightedText);
        } else {
            const qreal dim = (i == hovered_ && enabled) ? 0.15 : 0.45;
            textColor = mix(pal.color(QPalette::Text), pal.color(QPalette::PlaceholderText), dim);
        }
        if (!enabled) textColor = mix(textColor, pal.color(QPalette::Window), 0.4);
        p.setPen(textColor);
        p.drawText(cell, Qt::AlignCenter, segments_.at(i));
    }

    if (hasFocus() && enabled) {
        QColor ring = pal.color(QPalette::Highlight);
        ring.setAlphaF(0.85f);
        p.setPen(QPen(ring, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(1.0, 1.0, -1.0, -1.0), std::max(0.0, radius - 1.0),
                          std::max(0.0, radius - 1.0));
    }
}

bool SegmentedControl::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::HoverMove: {
        const auto* he = static_cast<QHoverEvent*>(e);
        const int idx = segmentAt(he->position().toPoint());
        if (idx != hovered_) {
            hovered_ = idx;
            update();
        }
        break;
    }
    case QEvent::HoverLeave:
        if (hovered_ != -1) {
            hovered_ = -1;
            update();
        }
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void SegmentedControl::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && isEnabled()) {
        const int idx = segmentAt(e->position().toPoint());
        if (idx >= 0) {
            setCurrentIndex(idx);
            return;
        }
    }
    QWidget::mousePressEvent(e);
}

void SegmentedControl::keyPressEvent(QKeyEvent* e) {
    if (!segments_.isEmpty()) {
        switch (e->key()) {
        case Qt::Key_Left:
        case Qt::Key_Up:
            setCurrentIndex(current_ - 1);
            return;
        case Qt::Key_Right:
        case Qt::Key_Down:
            setCurrentIndex(current_ + 1);
            return;
        case Qt::Key_Home:
            setCurrentIndex(0);
            return;
        case Qt::Key_End:
            setCurrentIndex(count() - 1);
            return;
        default:
            break;
        }
    }
    QWidget::keyPressEvent(e);
}

} // namespace livim
