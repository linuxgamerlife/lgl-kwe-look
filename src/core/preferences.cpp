#include "preferences.h"

#include "kwepaths.h"

#include <QDir>
#include <QSettings>

bool Preferences::operator==(const Preferences &o) const
{
    return exportSettingsIni == o.exportSettingsIni && exportGtkrc2 == o.exportGtkrc2
        && exportIndexTheme == o.exportIndexTheme && exportXsettingsd == o.exportXsettingsd
        && exportGtk4Symlinks == o.exportGtk4Symlinks
        && flatpakGtkThemeOverride == o.flatpakGtkThemeOverride
        && flatpakIconThemeOverride == o.flatpakIconThemeOverride
        && flatpakInstallCurrentTheme == o.flatpakInstallCurrentTheme && linkQt == o.linkQt;
}

namespace Prefs {

QString filePath()
{
    return KwePaths::appConfigDir() + QStringLiteral("/config.ini");
}

Preferences load()
{
    const QSettings s(filePath(), QSettings::IniFormat);
    Preferences p;
    p.exportSettingsIni = s.value(QStringLiteral("Export/settings_ini"), p.exportSettingsIni).toBool();
    p.exportGtkrc2 = s.value(QStringLiteral("Export/gtkrc_2_0"), p.exportGtkrc2).toBool();
    p.exportIndexTheme = s.value(QStringLiteral("Export/index_theme"), p.exportIndexTheme).toBool();
    p.exportXsettingsd = s.value(QStringLiteral("Export/xsettingsd"), p.exportXsettingsd).toBool();
    p.exportGtk4Symlinks = s.value(QStringLiteral("Export/gtk4_symlinks"), p.exportGtk4Symlinks).toBool();
    p.flatpakGtkThemeOverride =
        s.value(QStringLiteral("Flatpak/gtk_theme_override"), p.flatpakGtkThemeOverride).toBool();
    p.flatpakIconThemeOverride =
        s.value(QStringLiteral("Flatpak/icon_theme_override"), p.flatpakIconThemeOverride).toBool();
    p.flatpakInstallCurrentTheme =
        s.value(QStringLiteral("Flatpak/install_current_theme"), p.flatpakInstallCurrentTheme).toBool();
    p.linkQt = s.value(QStringLiteral("Qt/link_qt5_qt6"), p.linkQt).toBool();
    return p;
}

bool save(const Preferences &p, QString *error)
{
    QDir().mkpath(KwePaths::appConfigDir());
    QSettings s(filePath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Export/settings_ini"), p.exportSettingsIni);
    s.setValue(QStringLiteral("Export/gtkrc_2_0"), p.exportGtkrc2);
    s.setValue(QStringLiteral("Export/index_theme"), p.exportIndexTheme);
    s.setValue(QStringLiteral("Export/xsettingsd"), p.exportXsettingsd);
    s.setValue(QStringLiteral("Export/gtk4_symlinks"), p.exportGtk4Symlinks);
    s.setValue(QStringLiteral("Flatpak/gtk_theme_override"), p.flatpakGtkThemeOverride);
    s.setValue(QStringLiteral("Flatpak/icon_theme_override"), p.flatpakIconThemeOverride);
    s.setValue(QStringLiteral("Flatpak/install_current_theme"), p.flatpakInstallCurrentTheme);
    s.setValue(QStringLiteral("Qt/link_qt5_qt6"), p.linkQt);
    s.sync();
    if (s.status() != QSettings::NoError) {
        if (error)
            *error = QStringLiteral("cannot write %1").arg(filePath());
        return false;
    }
    return true;
}

QByteArray loadWindowGeometry()
{
    const QSettings s(filePath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Window/geometry")).toByteArray();
}

void saveWindowGeometry(const QByteArray &geometry)
{
    QDir().mkpath(KwePaths::appConfigDir());
    QSettings s(filePath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Window/geometry"), geometry);
}

QByteArray loadLayoutState(const QString &name)
{
    const QSettings s(filePath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Layout/%1").arg(name)).toByteArray();
}

void saveLayoutState(const QString &name, const QByteArray &state)
{
    QDir().mkpath(KwePaths::appConfigDir());
    QSettings s(filePath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Layout/%1").arg(name), state);
}

bool migrationDone()
{
    const QSettings s(filePath(), QSettings::IniFormat);
    return s.value(QStringLiteral("Migration/nwg_look_imported"), false).toBool();
}

void markMigrationDone()
{
    QDir().mkpath(KwePaths::appConfigDir());
    QSettings s(filePath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("Migration/nwg_look_imported"), true);
}

}  // namespace Prefs
