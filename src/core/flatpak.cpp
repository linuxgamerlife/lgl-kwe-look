// SPDX-License-Identifier: MPL-2.0
//
// Derived from stylepak (https://github.com/refi64/stylepak), MPL-2.0, via the Go
// translation in nwg-look (Eslam Allam). C++ translation for lgl-kwe-look.
#include "flatpak.h"

#include "gsettingsclient.h"
#include "kwepaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Flatpak {

namespace {

const char *const kMarker = ".lgl-kwe-look";
const char *const kLegacyMarker = ".nwg-look";   // themes installed by nwg-look are recognised

bool run(const QStringList &args, QString *err)
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("flatpak"));
    if (exe.isEmpty()) {
        *err = QStringLiteral("flatpak is not installed");
        return false;
    }
    QProcess p;
    p.start(exe, args);
    if (!p.waitForFinished(15000)) {
        p.kill();
        p.waitForFinished(1000);
        *err = QStringLiteral("flatpak timed out");
        return false;
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}

bool isManaged(const QString &themeDir)
{
    return QFileInfo::exists(themeDir + QLatin1Char('/') + QLatin1String(kMarker))
        || QFileInfo::exists(themeDir + QLatin1Char('/') + QLatin1String(kLegacyMarker));
}

QString findThemePath(const QString &theme)
{
    const QStringList roots{KwePaths::dataHome() + QStringLiteral("/themes"),
                            KwePaths::home() + QStringLiteral("/.themes"),
                            QStringLiteral("/usr/share/themes")};
    for (const QString &r : roots) {
        const QString full = r + QLatin1Char('/') + theme;
        if (QFileInfo(full).isDir())
            return full;
    }
    return QString();
}

// Recursive copy that keeps symlinks as symlinks, like stylepak's copyDir.
bool copyDir(const QString &src, const QString &dst, QString *err)
{
    if (!QDir().mkpath(dst)) {
        *err = QStringLiteral("cannot create %1").arg(dst);
        return false;
    }
    const QDir d(src);
    const auto entries = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System
                                         | QDir::Hidden);
    for (const QFileInfo &fi : entries) {
        const QString target = dst + QLatin1Char('/') + fi.fileName();
        if (fi.isSymLink()) {
            QFile::remove(target);
            if (!QFile::link(fi.symLinkTarget(), target)) {
                *err = QStringLiteral("cannot create symlink %1").arg(target);
                return false;
            }
        } else if (fi.isDir()) {
            if (!copyDir(fi.absoluteFilePath(), target, err))
                return false;
        } else {
            QFile::remove(target);
            if (!QFile::copy(fi.absoluteFilePath(), target)) {
                *err = QStringLiteral("cannot copy %1").arg(fi.absoluteFilePath());
                return false;
            }
        }
    }
    return true;
}

bool copyThemeFiles(const QString &src, const QString &dst, QString *err)
{
    int copied = 0;
    const QDir d(src);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!fi.fileName().startsWith(QLatin1String("gtk-")))
            continue;
        if (!copyDir(fi.absoluteFilePath(), dst + QLatin1Char('/') + fi.fileName(), err))
            return false;
        ++copied;
    }
    if (copied == 0) {
        *err = QStringLiteral("no gtk-* directories found in theme %1").arg(src);
        return false;
    }
    const QString index = src + QStringLiteral("/index.theme");
    if (QFileInfo::exists(index)) {
        const QString out = dst + QStringLiteral("/index.theme");
        QFile::remove(out);
        if (!QFile::copy(index, out)) {
            *err = QStringLiteral("cannot copy index.theme");
            return false;
        }
    }
    return true;
}

void removeStaleThemes(const QString &themesDir, const QString &current)
{
    const QDir d(themesDir);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (fi.fileName() == current || !isManaged(fi.absoluteFilePath()))
            continue;
        QDir(fi.absoluteFilePath()).removeRecursively();
    }
}

}  // namespace

bool available()
{
    return !QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty();
}

StepResult overrideEnv(const QString &name, const QString &value)
{
    QString err;
    if (!run({QStringLiteral("override"), QStringLiteral("--user"),
              QStringLiteral("--env=%1=%2").arg(name, value)}, &err))
        return StepResult::warn(QStringLiteral("flatpak override %1 failed: %2").arg(name, err));
    return StepResult::ok(QStringLiteral("%1=%2").arg(name, value));
}

StepResult unsetEnv(const QString &name)
{
    QString err;
    if (!run({QStringLiteral("override"), QStringLiteral("--user"),
              QStringLiteral("--unset-env=%1").arg(name)}, &err))
        return StepResult::warn(QStringLiteral("flatpak unset %1 failed: %2").arg(name, err));
    return StepResult::ok(QStringLiteral("%1 override removed").arg(name));
}

bool validThemeName(const QString &theme)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9._\\-]+$"));
    return re.match(theme).hasMatch();
}

StepResult installUserTheme(const QString &theme)
{
    if (!validThemeName(theme))
        return StepResult::warn(QStringLiteral("invalid theme name '%1'").arg(theme));

    const QString themesDir = KwePaths::home() + QStringLiteral("/.themes");
    const QString userDir = themesDir + QLatin1Char('/') + theme;

    const QString themePath = findThemePath(theme);
    if (themePath.isEmpty())
        return StepResult::warn(QStringLiteral("theme '%1' not found in the known theme folders").arg(theme));
    if (QFileInfo(themePath).canonicalFilePath() == QFileInfo(userDir).canonicalFilePath())
        return StepResult::ok(QStringLiteral("theme is already in ~/.themes and is user-managed"));

    removeStaleThemes(themesDir, theme);

    const bool exists = QFileInfo::exists(userDir);
    QString err;
    if (!exists || isManaged(userDir)) {
        if (!QDir().mkpath(userDir))
            return StepResult::fail(QStringLiteral("cannot create %1").arg(userDir));
        if (!copyThemeFiles(themePath, userDir, &err))
            return StepResult::fail(err);
        QFile marker(userDir + QLatin1Char('/') + QLatin1String(kMarker));
        if (marker.open(QIODevice::WriteOnly))
            marker.write("managed by lgl-kwe-look\n");
    }
    // else: exists and is not ours -> skip the copy, still grant access.

    if (!run({QStringLiteral("override"), QStringLiteral("--user"),
              QStringLiteral("--filesystem=%1:ro").arg(themesDir)}, &err))
        return StepResult::warn(QStringLiteral("flatpak override ~/.themes failed: %1").arg(err));

    return StepResult::ok(QStringLiteral("theme %1 available to Flatpak apps").arg(theme));
}

}  // namespace Flatpak
