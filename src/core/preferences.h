#pragma once
#include <QByteArray>
#include <QString>

// The app's own settings, kept in ~/.config/lgl-kwe-look/config.ini.
//
// Deliberately NOT in qt6ct.conf: upstream qt6ct saves its window geometry there,
// and the platform plugin's file watcher then reloads every running Qt app each
// time the settings window closes.
struct Preferences {
    // GTK export (nwg-look "preferences")
    bool exportSettingsIni = true;
    bool exportGtkrc2 = true;
    bool exportIndexTheme = true;
    bool exportXsettingsd = true;
    // Off by default: linking gtk-4.0 files interferes with Noctalia's gtk.css.
    bool exportGtk4Symlinks = false;

    bool flatpakGtkThemeOverride = false;
    bool flatpakIconThemeOverride = false;   // ICON_THEME is not a standard variable: opt-in
    bool flatpakInstallCurrentTheme = false;

    // Write qt5ct.conf and qt6ct.conf together, like the KineticWE session does.
    bool linkQt = true;

    bool operator==(const Preferences &o) const;
    bool operator!=(const Preferences &o) const { return !(*this == o); }
};

namespace Prefs {
QString filePath();
Preferences load();
bool save(const Preferences &p, QString *error);

QByteArray loadWindowGeometry();
void saveWindowGeometry(const QByteArray &geometry);

// Layout state such as splitter positions, by name. Empty when nothing was saved.
QByteArray loadLayoutState(const QString &name);
void saveLayoutState(const QString &name, const QByteArray &state);

bool migrationDone();
void markMigrationDone();
}  // namespace Prefs
