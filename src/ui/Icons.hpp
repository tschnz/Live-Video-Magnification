#pragma once

// Dependency-free QPainter icon factory (no QtSvg): line art on a 24x24 grid, rendered at the
// application's device pixel ratio.

#include <QColor>
#include <QIcon>

namespace livim::icons {

// Each builder returns a QIcon of `px` logical points; `color` tints the strokes/fills.

QIcon openFile(const QColor& color, int px = 18);     // folder
QIcon camera(const QColor& color, int px = 18);       // video camera
QIcon exportArrow(const QColor& color, int px = 18);  // arrow-down-to-tray (download)
QIcon play(const QColor& color, int px = 18);         // filled triangle
QIcon pause(const QColor& color, int px = 18);        // two filled rounded bars
QIcon stop(const QColor& color, int px = 18);         // filled rounded square
QIcon loop(const QColor& color, int px = 18);         // repeat / refresh-cw arrows
QIcon roi(const QColor& color, int px = 18);          // crosshair / target rectangle
QIcon reset(const QColor& color, int px = 18);        // rotate-counterclockwise
QIcon fullscreen(const QColor& color, int px = 18);   // maximize / expand corners
QIcon sliders(const QColor& color, int px = 18);      // two horizontal sliders (decorative)
QIcon sun(const QColor& color, int px = 18);          // light appearance: rayed disc
QIcon moon(const QColor& color, int px = 18);         // dark appearance: crescent

} // namespace livim::icons
