#include "session.h"

#include "gtkexport.h"
#include "kwepaths.h"
#include "themestate.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>

namespace Session {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Session", text);
}

QString cfg(const QString &rel)
{
    return KwePaths::realConfigHome() + QLatin1Char('/') + rel;
}

QString displayName(Type t, const QString &raw)
{
    switch (t) {
    case Type::KineticWE: return QStringLiteral("KineticWE");
    case Type::Kde:       return QStringLiteral("KDE");
    case Type::Gnome:     return QStringLiteral("GNOME");
    case Type::Xfce:      return QStringLiteral("Xfce");
    case Type::Cinnamon:  return QStringLiteral("Cinnamon");
    case Type::Mate:      return QStringLiteral("MATE");
    case Type::Lxqt:      return QStringLiteral("LXQt");
    case Type::Budgie:    return QStringLiteral("Budgie");
    case Type::Other:     break;
    }
    return raw.isEmpty() ? tr("Unknown desktop") : raw;
}

}  // namespace

Type typeFromDesktop(const QString &value)
{
    const QStringList parts = value.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    const auto has = [&parts](const char *token) {
        for (const QString &p : parts) {
            if (p.compare(QLatin1String(token), Qt::CaseInsensitive) == 0)
                return true;
        }
        return false;
    };

    // Order matters: Budgie reports "Budgie:GNOME".
    if (has("KineticWE"))
        return Type::KineticWE;
    if (has("KDE") || has("plasma") || has("plasmawayland"))
        return Type::Kde;
    if (has("Budgie"))
        return Type::Budgie;
    if (has("X-Cinnamon") || has("Cinnamon"))
        return Type::Cinnamon;
    if (has("GNOME") || has("GNOME-Classic") || has("GNOME-Flashback"))
        return Type::Gnome;
    if (has("XFCE"))
        return Type::Xfce;
    if (has("MATE"))
        return Type::Mate;
    if (has("LXQt"))
        return Type::Lxqt;
    return Type::Other;
}

Info detect()
{
    Info info;
    info.desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    // Some sessions only set DESKTOP_SESSION.
    const QString raw = info.desktop.isEmpty() ? qEnvironmentVariable("DESKTOP_SESSION") : info.desktop;
    info.type = typeFromDesktop(raw);
    info.name = displayName(info.type, raw);

    const QString st = qEnvironmentVariable("XDG_SESSION_TYPE").toLower();
    info.display = st == QLatin1String("wayland") ? QStringLiteral("Wayland")
                   : st == QLatin1String("x11")   ? QStringLiteral("X11")
                                                  : QString();
    return info;
}

bool writesKdeGlobals()
{
    return KwePaths::kweConfigPresent() || QFileInfo::exists(KwePaths::kdeglobals()) || detect().type == Type::Kde;
}

bool writesKcminputrc()
{
    return detect().type == Type::Kde;
}

bool mirrorsQtCt(QtTarget t)
{
    // KineticWE exports qt5ct/qt6ct, and window managers have no platform theme of their own.
    // A full desktop brings its own Qt platform theme, so qt5ct/qt6ct are only touched there
    // when they are actually set up or selected.
    switch (detect().type) {
    case Type::KineticWE:
    case Type::Other:
        return true;
    default:
        break;
    }
    return QFileInfo::exists(QtCt::configFile(t))
        || qEnvironmentVariable("QT_QPA_PLATFORMTHEME").contains(QtCt::toolName(t));
}

QList<Output> outputs(const Preferences &prefs)
{
    QList<Output> out;
    const auto file = [&out](Output::Group g, const QString &what, const QString &path) {
        Output o;
        o.group = g;
        o.what = what;
        o.path = path;
        o.exists = QFileInfo::exists(path);
        out << o;
    };
    const auto note = [&out](Output::Group g, const QString &what, const QString &desc) {
        Output o;
        o.group = g;
        o.what = what;
        o.path = desc;
        o.isFile = false;
        o.exists = true;
        out << o;
    };
    using G = Output::Group;

    // ---- desktop session ----
    if (KwePaths::kweConfigPresent()) {
        file(G::Desktop, tr("KineticWE icon and cursor"), KwePaths::appearanceKwe());
        file(G::Desktop, tr("KineticWE compositor cursor"), KwePaths::kineticweKwe());
    }
    if (writesKdeGlobals())
        file(G::Desktop, tr("KDE icon and cursor"), KwePaths::kdeglobals());
    if (writesKcminputrc())
        file(G::Desktop, tr("Plasma cursor theme"), KwePaths::kcminputrc());
    note(G::Desktop, tr("gsettings"), QStringLiteral("org.gnome.desktop.interface (dconf)"));

    // ---- GTK ----
    if (prefs.exportSettingsIni) {
        file(G::Gtk, tr("GTK3 settings"), cfg(QStringLiteral("gtk-3.0/settings.ini")));
        file(G::Gtk, tr("GTK4 settings"), cfg(QStringLiteral("gtk-4.0/settings.ini")));
    }
    if (prefs.exportGtkrc2)
        file(G::Gtk, tr("GTK2 settings"), GtkExport::gtkrc2Path());
    if (prefs.exportIndexTheme)
        file(G::Gtk, tr("Default cursor theme"), GtkExport::iconsFolderForIndexTheme() + QStringLiteral("/default/index.theme"));
    if (prefs.exportXsettingsd)
        file(G::Gtk, tr("xsettingsd"), cfg(QStringLiteral("xsettingsd/xsettingsd.conf")));
    if (prefs.exportGtk4Symlinks)
        file(G::Gtk, tr("GTK4 theme links"), cfg(QStringLiteral("gtk-4.0")));
    file(G::Gtk, tr("Settings backup"), ThemeStateIO::backupPath());

    // ---- Qt ----
    for (QtTarget t : {QtTarget::Qt5, QtTarget::Qt6})
        file(G::Qt, tr("%1 settings").arg(QtCt::toolName(t)), QtCt::configFile(t));

    return out;
}

}  // namespace Session
