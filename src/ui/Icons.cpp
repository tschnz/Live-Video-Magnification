#include "ui/Icons.hpp"

#include <cmath>
#include <functional>

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QRectF>
#include <QScreen>

namespace livim::icons {
namespace {

// Design grid every draw callback works in; the painter scales it into the requested pixel box.
constexpr qreal kGrid = 24.0;
// Stroke weight on the 24-grid.
constexpr qreal kStroke = 2.0;

// Primary screen's DPR; falls back to 1.0 when there is no QGuiApplication/screen yet.
qreal appDpr() {
    if (const QGuiApplication* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        if (const QScreen* screen = app->primaryScreen())
            return screen->devicePixelRatio();
    }
    return 1.0;
}

// Render a 24-grid drawing into a transparent pixmap of `px` logical points at the given DPR.
// The pen is preset to `color`; filled icons override brush/pen inside their callback.
QPixmap renderPixmap(int px, qreal dpr, const QColor& color,
                     const std::function<void(QPainter&)>& draw24) {
    const qreal logical = static_cast<qreal>(px);
    const int physical = static_cast<int>(std::lround(logical * dpr));

    QPixmap pm(physical, physical);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Map the 24-grid into the logical box.
    const qreal scale = logical / kGrid;
    p.scale(scale, scale);

    QPen pen(color);
    pen.setWidthF(kStroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    draw24(p);
    p.end();
    return pm;
}

QIcon makeIcon(int px, const QColor& color, const std::function<void(QPainter&)>& draw24) {
    QIcon icon;
    icon.addPixmap(renderPixmap(px, appDpr(), color, draw24));
    return icon;
}

void fillWith(QPainter& p, const QColor& color, const QPainterPath& path) {
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(path);
}

} // namespace

QIcon openFile(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QPainterPath path;
        path.moveTo(3.0, 7.0);
        path.lineTo(3.0, 19.0);
        path.lineTo(21.0, 19.0);
        path.lineTo(21.0, 9.0);
        path.lineTo(11.0, 9.0);
        path.lineTo(9.0, 6.0);
        path.lineTo(4.0, 6.0);
        path.closeSubpath();
        p.drawPath(path);
    });
}

QIcon camera(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QRectF body(3.0, 7.0, 12.0, 10.0);
        p.drawRoundedRect(body, 2.0, 2.0);
        QPolygonF lens;
        lens << QPointF(15.0, 10.5) << QPointF(21.0, 7.5) << QPointF(21.0, 16.5)
             << QPointF(15.0, 13.5);
        p.drawPolyline(lens);
    });
}

QIcon exportArrow(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QPainterPath tray;
        tray.moveTo(4.0, 15.0);
        tray.lineTo(4.0, 19.0);
        tray.lineTo(20.0, 19.0);
        tray.lineTo(20.0, 15.0);
        p.drawPath(tray);
        p.drawLine(QPointF(12.0, 4.0), QPointF(12.0, 14.0));
        QPolygonF head;
        head << QPointF(8.0, 10.0) << QPointF(12.0, 14.0) << QPointF(16.0, 10.0);
        p.drawPolyline(head);
    });
}

QIcon play(const QColor& color, int px) {
    return makeIcon(px, color, [color](QPainter& p) {
        QPainterPath tri;
        tri.moveTo(8.0, 5.0);
        tri.lineTo(19.0, 12.0);
        tri.lineTo(8.0, 19.0);
        tri.closeSubpath();
        // Round the sharp tips.
        QPainterPathStroker stroker;
        stroker.setWidth(kStroke);
        stroker.setJoinStyle(Qt::RoundJoin);
        stroker.setCapStyle(Qt::RoundCap);
        const QPainterPath rounded = tri.united(stroker.createStroke(tri));
        fillWith(p, color, rounded);
    });
}

QIcon pause(const QColor& color, int px) {
    return makeIcon(px, color, [color](QPainter& p) {
        QPainterPath bars;
        bars.addRoundedRect(QRectF(7.0, 5.0, 3.0, 14.0), 1.5, 1.5);
        bars.addRoundedRect(QRectF(14.0, 5.0, 3.0, 14.0), 1.5, 1.5);
        fillWith(p, color, bars);
    });
}

QIcon stop(const QColor& color, int px) {
    return makeIcon(px, color, [color](QPainter& p) {
        QPainterPath sq;
        sq.addRoundedRect(QRectF(6.0, 6.0, 12.0, 12.0), 2.5, 2.5);
        fillWith(p, color, sq);
    });
}

QIcon loop(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QRectF box(5.0, 5.0, 14.0, 14.0);
        p.drawArc(box, 30 * 16, 130 * 16);
        QPolygonF headTop;
        headTop << QPointF(5.5, 11.0) << QPointF(5.0, 6.5) << QPointF(9.5, 7.0);
        p.drawPolyline(headTop);
        p.drawArc(box, 210 * 16, 130 * 16);
        QPolygonF headBot;
        headBot << QPointF(18.5, 13.0) << QPointF(19.0, 17.5) << QPointF(14.5, 17.0);
        p.drawPolyline(headBot);
    });
}

QIcon roi(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        p.drawRoundedRect(QRectF(4.0, 4.0, 16.0, 16.0), 2.0, 2.0);
        p.drawLine(QPointF(12.0, 4.0), QPointF(12.0, 9.0));
        p.drawLine(QPointF(12.0, 15.0), QPointF(12.0, 20.0));
        p.drawLine(QPointF(4.0, 12.0), QPointF(9.0, 12.0));
        p.drawLine(QPointF(15.0, 12.0), QPointF(20.0, 12.0));
    });
}

QIcon reset(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QRectF box(5.0, 5.0, 14.0, 14.0);
        // Sweep most of the circle, leaving a gap at the top-left for the arrowhead.
        p.drawArc(box, 110 * 16, 310 * 16);
        QPolygonF head;
        head << QPointF(4.0, 8.5) << QPointF(7.0, 5.0) << QPointF(10.5, 7.5);
        p.drawPolyline(head);
    });
}

QIcon fullscreen(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        const qreal a = 4.0, b = 20.0, t = 8.0; // outer corners and tick length
        p.drawPolyline(QPolygonF() << QPointF(t, a) << QPointF(a, a) << QPointF(a, t));
        p.drawPolyline(QPolygonF() << QPointF(b - (t - a), a) << QPointF(b, a) << QPointF(b, t));
        p.drawPolyline(QPolygonF() << QPointF(b, b - (t - a)) << QPointF(b, b)
                                   << QPointF(b - (t - a), b));
        p.drawPolyline(QPolygonF() << QPointF(a, b - (t - a)) << QPointF(a, b) << QPointF(t, b));
    });
}

QIcon sun(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        constexpr qreal kPi = 3.14159265358979323846;
        p.drawEllipse(QPointF(12.0, 12.0), 4.0, 4.0);
        for (int k = 0; k < 8; ++k) {
            const qreal a = static_cast<qreal>(k) * kPi / 4.0;
            const qreal dx = std::cos(a), dy = std::sin(a);
            p.drawLine(QPointF(12.0 + dx * 6.6, 12.0 + dy * 6.6),
                       QPointF(12.0 + dx * 9.3, 12.0 + dy * 9.3));
        }
    });
}

QIcon moon(const QColor& color, int px) {
    return makeIcon(px, color, [](QPainter& p) {
        QPainterPath full;
        full.addEllipse(QPointF(11.5, 12.0), 8.0, 8.0);
        QPainterPath bite;
        bite.addEllipse(QPointF(16.2, 8.4), 7.6, 7.6);
        p.drawPath(full.subtracted(bite));
    });
}

QIcon sliders(const QColor& color, int px) {
    return makeIcon(px, color, [color](QPainter& p) {
        p.drawLine(QPointF(4.0, 8.0), QPointF(20.0, 8.0));
        p.drawLine(QPointF(4.0, 16.0), QPointF(20.0, 16.0));
        // Filled so the knobs read at small sizes.
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(9.0, 8.0), 2.6, 2.6);
        p.drawEllipse(QPointF(15.0, 16.0), 2.6, 2.6);
    });
}

} // namespace livim::icons
