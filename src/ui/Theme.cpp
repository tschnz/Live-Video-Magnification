#include "ui/Theme.hpp"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPolygonF>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QStyleHints>

namespace livim {
namespace theme {
namespace {

// QSS template; @tokens are substituted from the active ThemePalette in styleSheet().
constexpr const char* kStyleTemplate = R"QSS(
QToolTip {
    background: @surface2;
    color: @text;
    border: 1px solid @line;
    border-radius: @radiusSmall;
    padding: 5px 8px;
}

/* --- Buttons --- */
QPushButton {
    background: @raised;
    color: @text;
    border: 1px solid @line;
    border-radius: @radiusSmall;
    padding: 7px 13px;
}
QPushButton:hover    { border-color: @accent; }
QPushButton:pressed  { background: @surface2; }
QPushButton:checked  { background: @accent; color: @accentInk; border-color: transparent; }
QPushButton:disabled { color: @faint; border-color: @line; background: @surface; }
/* Primary action: set the dynamic property `accent` = true on the widget. */
QPushButton[accent="true"]          { background: @accent; color: @accentInk; border: none; padding: 8px 14px; }
QPushButton[accent="true"]:hover    { background: @accent; }
QPushButton[accent="true"]:disabled { background: @surface2; color: @faint; }

/* --- Combo / spin / line edits --- */
QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {
    background: @field;
    color: @text;
    border: 1px solid @line;
    border-radius: @radiusSmall;
    padding: 6px 9px;
    selection-background-color: @accent;
    selection-color: @accentInk;
}
QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QLineEdit:hover { border-color: @accent; }
QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QLineEdit:focus { border-color: @accent; }
QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled { color: @faint; }
QComboBox::drop-down { border: none; width: 20px; }
/* Our combo styling otherwise trips Qt's scrolling-popup mode, which clipped 3-item combos to
   2-of-3. combobox-popup: 0 lists items up to maxVisibleItems instead, with no scroller. */
QComboBox { combobox-popup: 0; }
/* Styling ::drop-down suppresses the native arrow, so supply a themed chevron; styleSheet()
   generates it to a cache file and substitutes the path for @chevron. */
QComboBox::down-arrow { image: url("@chevron"); width: 12px; height: 12px; }
QComboBox QAbstractItemView {
    background: @surface2;
    color: @text;
    border: 1px solid @line;
    border-radius: @radiusSmall;
    selection-background-color: @accent;
    selection-color: @accentInk;
    outline: none;
    padding: 4px;
}

/* --- Check / radio --- */
QCheckBox, QRadioButton { color: @text; spacing: 8px; }
QCheckBox::indicator, QRadioButton::indicator { width: 16px; height: 16px; }
QCheckBox::indicator {
    border: 1px solid @line; border-radius: 4px; background: @field;
}
QCheckBox::indicator:hover    { border-color: @accent; }
QCheckBox::indicator:checked  { background: @accent; border-color: transparent; }
QCheckBox:disabled            { color: @faint; }

/* --- Inspector labels --- */
QLabel#sectionHeading { color: @dim; font-weight: 700; }
QLabel#fieldLabel { color: @dim; }

/* --- Group box (inspector sections) --- */
QGroupBox {
    border: 1px solid @line;
    border-radius: @radius;
    margin-top: 14px;
    padding: 10px 10px 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
    color: @dim;
    font-weight: 700;
}

/* --- Sliders --- */
QSlider::groove:horizontal { height: 5px; border-radius: 3px; background: @line; }
QSlider::sub-page:horizontal { height: 5px; border-radius: 3px; background: @accent; }
QSlider::handle:horizontal {
    width: 15px; height: 15px; margin: -6px 0; border-radius: 8px;
    background: #FFFFFF; border: 1px solid @line;
}
QSlider::handle:horizontal:hover { border-color: @accent; }

/* --- Progress (export) --- */
QProgressBar {
    background: @field; border: 1px solid @line; border-radius: @radiusSmall;
    text-align: center; color: @text; height: 16px;
}
QProgressBar::chunk { background: @accent; border-radius: 5px; }

/* --- Lists (camera picker) --- */
QListWidget {
    background: @field; border: 1px solid @line; border-radius: @radiusSmall; outline: none;
}
QListWidget::item { padding: 7px 9px; border-radius: @radiusSmall; }
QListWidget::item:selected { background: @accent; color: @accentInk; }
QListWidget::item:hover:!selected { background: @surface2; }

/* --- Separators (HLine = shape 4, VLine = shape 5; constrain only the thin axis) --- */
QFrame[frameShape="4"] { background: @line; border: none; max-height: 1px; }
QFrame[frameShape="5"] { background: @line; border: none; max-width: 1px; }

/* --- Scrollbars --- */
QScrollBar:vertical   { background: transparent; width: 11px; margin: 0; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }
QScrollBar::handle:vertical   { background: @line; border-radius: 5px; min-height: 28px; }
QScrollBar::handle:horizontal { background: @line; border-radius: 5px; min-width: 28px; }
QScrollBar::handle:hover { background: @faint; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* --- Menus / dialogs --- */
QMenu { background: @surface2; color: @text; border: 1px solid @line; border-radius: @radius; padding: 5px; }
QMenu::item { padding: 6px 22px; border-radius: @radiusSmall; }
QMenu::item:selected { background: @accent; color: @accentInk; }
QDialog { background: @bg; }

/* --- Spin step buttons: slim and themed (arrows stay Fusion-drawn) --- */
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border; width: 16px; background: @surface2; border: none;
}
QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: @radiusSmall; }
QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-position: bottom right; border-bottom-right-radius: @radiusSmall; }
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: @raised; }

/* --- Round transport buttons (Play/Pause/Stop) --- */
QPushButton#transportBtn { border-radius: 17px; padding: 0; }

/* --- Numeric readout chips (SliderRow) and the stats strip: monospace, tabular --- */
QLabel#valueReadout, QLineEdit#valueReadout, QComboBox#valueReadout, QDoubleSpinBox#valueReadout {
    color: @text; background: @field; border: 1px solid @line; border-radius: @radiusSmall;
    padding: 1px 6px;
    font-size: 11px;
    font-family: "DejaVu Sans Mono", "Cascadia Code", "Consolas", "Menlo", monospace;
}
/* An ID selector outranks the generic QLineEdit:focus / QComboBox:focus rules, so the accent
   border has to be restated here. */
QLineEdit#valueReadout:hover, QComboBox#valueReadout:hover, QDoubleSpinBox#valueReadout:hover { border-color: @accent; }
QLineEdit#valueReadout:focus, QComboBox#valueReadout:focus, QDoubleSpinBox#valueReadout:focus { border-color: @accent; }
QLineEdit#valueReadout:disabled, QComboBox#valueReadout:disabled, QDoubleSpinBox#valueReadout:disabled { color: @faint; }
QComboBox#valueReadout::drop-down { border: none; width: 16px; }
/* A styled combo's tight field padding otherwise collapses the popup's item heights so options
   overlap and some are not listed. Unqualified on purpose: the popup view is not a QSS descendant
   of the combo by objectName, so a `#valueReadout`-scoped rule would not reach it. */
QComboBox QAbstractItemView::item { min-height: 24px; }
/* The frequency band's beats-per-minute companion, secondary to the Hz chips below it. */
QLabel#freqSubLabel {
    color: @dim;
    font-size: 11px;
    font-family: "DejaVu Sans Mono", "Cascadia Code", "Consolas", "Menlo", monospace;
}
/* --- Status strip --- */
QWidget#statusStrip { background: @surface; border-top: 1px solid @line; }
QWidget#statSep     { background: @line; }
QLabel#statCaption  { color: @dim; }
/* A CSS-drawn circle whose fill is the health colour, set via the `state` property. */
QLabel#statDot {
    min-width: 8px; max-width: 8px; min-height: 8px; max-height: 8px; border-radius: 4px;
    background: @faint;
}
QLabel#statDot[state="ok"]   { background: @ok; }
QLabel#statDot[state="warn"] { background: @accent; }
QLabel#statDot[state="bad"]  { background: @danger; }
QLabel#statDot[state="idle"] { background: @faint; }
/* Calm @text when healthy, so colour only ever signals an exception. */
QLabel#statValue               { color: @text; }
QLabel#statValue[state="warn"] { color: @accent; }
QLabel#statValue[state="bad"]  { color: @danger; }
QLabel#statValue[state="idle"] { color: @faint; }
QLabel#statHint { color: @danger; padding-left: 2px; }
QLabel#statNotice { color: @accent; padding-left: 8px; }
/* The Playback FPS input embedded in the strip: shared field style, slimmer padding. */
QDoubleSpinBox#statSpin { padding: 1px 6px; }
QLabel#statSlash        { color: @dim; }

/* --- Toolbar --- */
QToolBar#mainToolBar { background: @surface; border: none; border-bottom: 1px solid @line; spacing: 8px; padding: 7px 10px; }
QToolBar#mainToolBar::separator { background: @line; width: 1px; margin: 5px 6px; }
QLabel#toolbarCaption { color: @dim; }

/* --- Splitter handle between the video and the inspector --- */
QSplitter#mainSplitter::handle { background: @line; }
QSplitter#mainSplitter::handle:hover { background: @accent; }
)QSS";

QString hex(const QColor& c) { return c.name(QColor::HexRgb); }

// Session appearance state (GUI thread only).
ColorScheme g_applied = ColorScheme::Dark;
bool g_following = true;

} // namespace

ThemePalette palette(ColorScheme scheme) {
    ThemePalette p;
    if (scheme == ColorScheme::Dark) {
        p.bg        = QColor("#15110D"); // warm espresso
        p.surface   = QColor("#211A14");
        p.surface2  = QColor("#29211A");
        p.raised    = QColor("#2C241C");
        p.line      = QColor("#382E25");
        p.text      = QColor("#F3ECE3");
        p.dim       = QColor("#A99A8B");
        p.faint     = QColor("#6E6359");
        p.field     = QColor("#0F0C09");
        p.accent    = QColor("#F4A23C"); // ember amber
        p.accent2   = QColor("#F0476E"); // rose (gradients only)
        p.accentInk = QColor("#2A1505"); // dark ink on the amber accent
        p.ok        = QColor("#8FCB8A");
        p.danger    = QColor("#F2606B");
    } else {
        p.bg        = QColor("#EEF0F2"); // cool porcelain
        p.surface   = QColor("#FFFFFF");
        p.surface2  = QColor("#F4F6F8");
        p.raised    = QColor("#FFFFFF");
        p.line      = QColor("#D8DCE0");
        p.text      = QColor("#1E1B17");
        p.dim       = QColor("#6B6A66");
        p.faint     = QColor("#9DA0A6");
        p.field     = QColor("#FFFFFF");
        p.accent    = QColor("#B8521C"); // burnt terracotta
        p.accent2   = QColor("#B01E5B"); // deep rose
        p.accentInk = QColor("#FFFFFF");
        p.ok        = QColor("#2E9E63");
        p.danger    = QColor("#C8473E");
    }
    return p;
}

QString styleSheet(const ThemePalette& p) {
    QString s = QString::fromUtf8(kStyleTemplate);
    s.replace("@bg", hex(p.bg));
    s.replace("@surface2", hex(p.surface2)); // before @surface so the longer token wins
    s.replace("@surface", hex(p.surface));
    s.replace("@raised", hex(p.raised));
    s.replace("@line", hex(p.line));
    s.replace("@text", hex(p.text));
    s.replace("@dim", hex(p.dim));
    s.replace("@faint", hex(p.faint));
    s.replace("@field", hex(p.field));
    s.replace("@accentInk", hex(p.accentInk)); // before @accent2/@accent
    s.replace("@accent2", hex(p.accent2));
    s.replace("@accent", hex(p.accent));
    s.replace("@ok", hex(p.ok));
    s.replace("@danger", hex(p.danger));
    s.replace("@radiusSmall", QString::number(metrics::radiusSmall) + "px");
    s.replace("@radius", QString::number(metrics::radius) + "px");

    // QSS url() needs a real file, so paint the chevron and cache it to disk. QImage, not QPixmap:
    // a QPixmap can come back null this early, before any window exists.
    QImage arrow(24, 24, QImage::Format_ARGB32_Premultiplied);
    arrow.fill(Qt::transparent);
    {
        QPainter ap(&arrow);
        ap.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(p.dim, 2.4);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        ap.setPen(pen);
        QPolygonF chevron;
        chevron << QPointF(7, 10) << QPointF(12, 15) << QPointF(17, 10);
        ap.drawPolyline(chevron);
    }
    // Must stay XPM: this Qt build has no PNG/JPEG codecs; XPM is a QtGui built-in.
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    QString arrowPath = cacheDir + "/livim-combo-arrow.xpm";
    if (!arrow.save(arrowPath, "XPM")) arrowPath.clear(); // better no arrow than a broken path
    s.replace("@chevron", arrowPath);
    return s;
}

void apply(QApplication& app, ColorScheme scheme) {
    const ThemePalette p = palette(scheme);

    // Fusion is identical on every OS and fully honours QSS.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    const bool dark = scheme == ColorScheme::Dark;
    QPalette pal;
    pal.setColor(QPalette::Window, p.bg);
    pal.setColor(QPalette::WindowText, p.text);
    pal.setColor(QPalette::Base, p.field);
    pal.setColor(QPalette::AlternateBase, p.surface2);
    pal.setColor(QPalette::Text, p.text);
    pal.setColor(QPalette::Button, p.raised);
    pal.setColor(QPalette::ButtonText, p.text);
    pal.setColor(QPalette::BrightText, p.danger);
    pal.setColor(QPalette::Highlight, p.accent);
    pal.setColor(QPalette::HighlightedText, p.accentInk);
    pal.setColor(QPalette::ToolTipBase, p.surface2);
    pal.setColor(QPalette::ToolTipText, p.text);
    pal.setColor(QPalette::PlaceholderText, p.faint);
    pal.setColor(QPalette::Link, p.accent);
    pal.setColor(QPalette::Mid, p.line);
    pal.setColor(QPalette::Dark, dark ? p.bg.darker(120) : p.line);
    pal.setColor(QPalette::Shadow, dark ? QColor("#05070C") : QColor("#C2CAD6"));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, p.faint);
    pal.setColor(QPalette::Disabled, QPalette::Text, p.faint);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, p.faint);
    app.setPalette(pal);

    // OS-provided faces only: SF Pro / Segoe are not redistributable. Qt picks the first that resolves.
    QFont uiFont = app.font();
    uiFont.setFamilies({QStringLiteral("Inter"), QStringLiteral("SF Pro Text"),
                        QStringLiteral("Segoe UI Variable Text"), QStringLiteral("Segoe UI"),
                        QStringLiteral("Cantarell"), QStringLiteral("Noto Sans"),
                        QStringLiteral("Ubuntu"), QStringLiteral("DejaVu Sans")});
    uiFont.setPointSizeF(10.0);
    app.setFont(uiFont);

    app.setStyleSheet(styleSheet(p));
    g_applied = scheme;
}

ColorScheme systemScheme() {
    if (auto* hints = QGuiApplication::styleHints())
        if (hints->colorScheme() == Qt::ColorScheme::Light) return ColorScheme::Light;
    return ColorScheme::Dark;
}

ColorScheme appliedScheme() { return g_applied; }

bool followingSystem() { return g_following; }

void overrideScheme(QApplication& app, ColorScheme scheme) {
    g_following = false;
    apply(app, scheme);
}

} // namespace theme
} // namespace livim
