#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QStyleHints>
#include <QSurfaceFormat>

#include "ui/MainWindow.hpp"
#include "ui/Theme.hpp"

int main(int argc, char** argv) {
    // GL 3.3 core matches the DisplayWidget's #version 330 core shaders; swap interval 1 = vsync.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 3);
    fmt.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(fmt);

    // Prefer the desktop's native file picker (xdg-desktop-portal) when a session bus is present.
    // Only Linux desktops set DBUS_SESSION_BUS_ADDRESS, so this is a no-op elsewhere.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME")
        && !qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS")) {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }

    QApplication app(argc, argv);

    // Disable freedesktop icon-theme lookups: the desktop's theme icons render inconsistently
    // against our QSS palette, and a theme icon Qt cannot decode yields a null pixmap behind a
    // non-null QIcon, which crashes Qt's own file dialog. All app icons are drawn in ui/Icons.cpp.
    QIcon::setThemeSearchPaths({});
    QIcon::setFallbackSearchPaths({});
    QIcon::setThemeName(QString());
    QIcon::setFallbackThemeName(QString());

    // Follow the OS light/dark appearance, live, until the user pins a theme via the toolbar toggle.
    livim::theme::apply(app, livim::theme::systemScheme());
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &app,
                     [&app](Qt::ColorScheme) {
                         if (livim::theme::followingSystem())
                             livim::theme::apply(app, livim::theme::systemScheme());
                     });

    livim::MainWindow window;
    window.resize(1280, 760);
    // Re-apply once with widgets present so QSS-driven size hints settle before the first show.
    livim::theme::apply(app, livim::theme::appliedScheme());
    window.show();

    return app.exec();
}
