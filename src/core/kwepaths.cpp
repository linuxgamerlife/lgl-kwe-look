#include "kwepaths.h"

#include <QDir>
#include <QFileInfo>

namespace KwePaths {

QString home()
{
    const QString h = qEnvironmentVariable("HOME");
    return h.isEmpty() ? QDir::homePath() : h;
}

QString realConfigHome()
{
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    return xdg.isEmpty() ? home() + QStringLiteral("/.config") : xdg;
}

QString kweConfigHome()
{
    const QString kwe = qEnvironmentVariable("KWE_CONFIG_HOME");
    return kwe.isEmpty() ? home() + QStringLiteral("/.config/kineticwe") : kwe;
}

QString dataHome()
{
    const QString xdg = qEnvironmentVariable("XDG_DATA_HOME");
    return xdg.isEmpty() ? home() + QStringLiteral("/.local/share") : xdg;
}

QStringList dataDirs()
{
    QStringList candidates;
    candidates << dataHome();
    const QString xdg = qEnvironmentVariable("XDG_DATA_DIRS");
    const QString dirs = xdg.isEmpty() ? QStringLiteral("/usr/local/share/:/usr/share/") : xdg;
    candidates << dirs.split(QLatin1Char(':'), Qt::SkipEmptyParts);

    QStringList out;
    for (const QString &d : std::as_const(candidates)) {
        if (QFileInfo::exists(d) && !out.contains(d))
            out << d;
    }
    return out;
}

QString appearanceKwe() { return kweConfigHome() + QStringLiteral("/appearance.kwe"); }
QString kineticweKwe()  { return kweConfigHome() + QStringLiteral("/kineticwe.kwe"); }
QString kdeglobals()    { return realConfigHome() + QStringLiteral("/kdeglobals"); }
QString kcminputrc()    { return realConfigHome() + QStringLiteral("/kcminputrc"); }

QString appConfigDir() { return realConfigHome() + QStringLiteral("/lgl-kwe-look"); }
QString appDataDir()   { return dataHome() + QStringLiteral("/lgl-kwe-look"); }

bool isKineticWeSession()
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP")
        .split(QLatin1Char(':'), Qt::SkipEmptyParts)
        .contains(QStringLiteral("KineticWE"), Qt::CaseInsensitive);
}

bool kweConfigPresent()
{
    return QFileInfo(kweConfigHome()).isDir();
}

}  // namespace KwePaths
