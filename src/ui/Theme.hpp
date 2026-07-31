#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace livim {

enum class ColorScheme { Dark, Light };

// The app's design tokens; every colour in the UI comes from here.
struct ThemePalette {
    QColor bg;        // window / canvas chrome ground
    QColor surface;   // toolbar / inspector panels
    QColor surface2;  // transport bar, sunken rows
    QColor raised;    // default buttons
    QColor line;      // hairline borders / separators
    QColor text;
    QColor dim;       // secondary text / labels
    QColor faint;     // tertiary text / disabled
    QColor field;     // text-entry background
    QColor accent;
    QColor accent2;   // gradient partner — gradients only, never flat chrome
    QColor accentInk; // text/icon colour on top of an accent fill
    QColor ok;
    QColor danger;
};

// Spacing scale (8pt grid) and corner radii, in px.
namespace metrics {
inline constexpr int space1 = 4;
inline constexpr int space2 = 8;
inline constexpr int space3 = 12;
inline constexpr int space4 = 16;
inline constexpr int space5 = 24;
inline constexpr int radius = 8;
inline constexpr int radiusSmall = 6;
} // namespace metrics

namespace theme {

// t=0 -> a, t=1 -> b.
inline QColor mix(const QColor& a, const QColor& b, qreal t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    const qreal u = 1.0 - t;
    return QColor(static_cast<int>(a.red() * u + b.red() * t),
                  static_cast<int>(a.green() * u + b.green() * t),
                  static_cast<int>(a.blue() * u + b.blue() * t),
                  static_cast<int>(a.alpha() * u + b.alpha() * t));
}

ThemePalette palette(ColorScheme scheme);

// Global QSS, layered on top of the Fusion base palette.
QString styleSheet(const ThemePalette& p);

// Sets the Fusion style, the QPalette and the QSS for the whole application.
void apply(QApplication& app, ColorScheme scheme);

// OS appearance via QStyleHints::colorScheme(); falls back to Dark when unknown.
ColorScheme systemScheme();

// The app follows the OS appearance until the user pins a scheme. Nothing is persisted.
ColorScheme appliedScheme();
bool followingSystem();
void overrideScheme(QApplication& app, ColorScheme scheme);

} // namespace theme
} // namespace livim
