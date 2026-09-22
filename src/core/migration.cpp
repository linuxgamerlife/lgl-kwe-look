#include "migration.h"

#include "inifile.h"
#include "kwepaths.h"
#include "themestate.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace Migration {

namespace {
QString nwgConfigPath() { return KwePaths::realConfigHome() + QStringLiteral("/nwg-look/config"); }
QString nwgBackupPath() { return KwePaths::dataHome() + QStringLiteral("/nwg-look/gsettings"); }
}  // namespace

bool needed()
{
    if (Prefs::migrationDone())
        return false;
    return QFileInfo::exists(nwgConfigPath()) || QFileInfo::exists(nwgBackupPath());
}

QStringList applyNwgLookJson(const QByteArray &json, Preferences *prefs, bool importGtk4Symlinks)
{
    QStringList applied;
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    const auto take = [&](const char *key, bool *target) {
        const QString k = QLatin1String(key);
        if (o.contains(k) && o.value(k).isBool()) {
            *target = o.value(k).toBool();
            applied << k;
        }
    };
    take("export-settings-ini", &prefs->exportSettingsIni);
    take("export-gtkrc-20", &prefs->exportGtkrc2);
    take("export-index-theme", &prefs->exportIndexTheme);
    take("export-xsettingsd", &prefs->exportXsettingsd);
    if (importGtk4Symlinks)
        take("export-gtk4-symlinks", &prefs->exportGtk4Symlinks);
    take("flatpak-export-gtk-theme-override", &prefs->flatpakGtkThemeOverride);
    take("flatpak-export-icon-theme-override", &prefs->flatpakIconThemeOverride);
    take("flatpak-install-current-gtk-theme", &prefs->flatpakInstallCurrentTheme);
    return applied;
}

Report run(Preferences *prefs)
{
    Report r;
    r.ran = true;

    QFile cfg(nwgConfigPath());
    if (cfg.open(QIODevice::ReadOnly)) {
        // nwg-look defaults GTK4 symlinks to on; in a KineticWE session that would
        // replace Noctalia's gtk.css, so it is not carried over there.
        const bool gtk4 = !KwePaths::isKineticWeSession();
        const QStringList applied = applyNwgLookJson(cfg.readAll(), prefs, gtk4);
        r.notes << QStringLiteral("imported %1 export preference(s) from nwg-look").arg(applied.size());
        if (!gtk4)
            r.notes << QStringLiteral("GTK4 symlink export left off (KineticWE session)");
        QString err;
        if (!Prefs::save(*prefs, &err))
            r.notes << err;
    }

    if (QFileInfo::exists(nwgBackupPath()) && !QFileInfo::exists(ThemeStateIO::backupPath())) {
        QDir().mkpath(KwePaths::appDataDir());
        if (QFile::copy(nwgBackupPath(), ThemeStateIO::backupPath()))
            r.notes << QStringLiteral("copied the nwg-look gsettings backup");
    }

    Prefs::markMigrationDone();
    return r;
}

}  // namespace Migration
