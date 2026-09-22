#include "gtkexport.h"

#include "inifile.h"
#include "kwepaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace GtkExport {

namespace {

QString one(bool v) { return v ? QStringLiteral("1") : QStringLiteral("0"); }

QString joinLines(const QStringList &lines)
{
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString cfg(const QString &rel) { return KwePaths::realConfigHome() + QLatin1Char('/') + rel; }

bool isThemeLink(const QString &path)
{
    const QFileInfo fi(path);
    return fi.isSymLink() && fi.symLinkTarget().contains(QLatin1String("/themes/"));
}

}  // namespace

QString hintStyle(const QString &h)
{
    if (h == QLatin1String("slight")) return QStringLiteral("hintslight");
    if (h == QLatin1String("medium")) return QStringLiteral("hintmedium");
    if (h == QLatin1String("full"))   return QStringLiteral("hintfull");
    return QStringLiteral("hintnone");
}

QStringList settingsIni3(const ThemeState &s, const QStringList &preservedLines)
{
    QStringList l{QStringLiteral("[Settings]")};
    l << QStringLiteral("gtk-theme-name=") + s.gtkTheme
      << QStringLiteral("gtk-icon-theme-name=") + s.iconTheme
      << QStringLiteral("gtk-font-name=") + s.fontName
      << QStringLiteral("gtk-cursor-theme-name=") + s.cursorTheme
      << QStringLiteral("gtk-cursor-theme-size=") + QString::number(s.cursorSize)
      << QStringLiteral("gtk-toolbar-style=") + s.iniToolbarStyle
      << QStringLiteral("gtk-toolbar-icon-size=") + s.iniToolbarIconSize
      << QStringLiteral("gtk-button-images=") + one(s.buttonImages)
      << QStringLiteral("gtk-menu-images=") + one(s.menuImages)
      << QStringLiteral("gtk-enable-event-sounds=") + one(s.eventSounds)
      << QStringLiteral("gtk-enable-input-feedback-sounds=") + one(s.inputFeedbackSounds)
      << QStringLiteral("gtk-xft-antialias=") + one(s.fontAntialiasing != QLatin1String("none"))
      << QStringLiteral("gtk-xft-hinting=") + one(s.fontHinting != QLatin1String("none"))
      << QStringLiteral("gtk-xft-hintstyle=") + hintStyle(s.fontHinting)
      << QStringLiteral("gtk-xft-rgba=") + s.fontRgbaOrder
      << QStringLiteral("gtk-application-prefer-dark-theme=")
             + one(s.colorScheme == QLatin1String("prefer-dark"));
    for (const QString &p : preservedLines) {
        if (!p.isEmpty())
            l << p;
    }
    return l;
}

QStringList settingsIni4(const ThemeState &s)
{
    return {QStringLiteral("[Settings]"),
            QStringLiteral("gtk-theme-name=") + s.gtkTheme,
            QStringLiteral("gtk-icon-theme-name=") + s.iconTheme,
            QStringLiteral("gtk-font-name=") + s.fontName,
            QStringLiteral("gtk-cursor-theme-name=") + s.cursorTheme,
            QStringLiteral("gtk-cursor-theme-size=") + QString::number(s.cursorSize),
            QStringLiteral("gtk-application-prefer-dark-theme=")
                + one(s.colorScheme == QLatin1String("prefer-dark"))};
}

QStringList gtkrc2(const ThemeState &s, const QString &home, const QString &generator)
{
    const auto q = [](const QString &v) { return QLatin1Char('"') + v + QLatin1Char('"'); };
    QStringList l{
        QStringLiteral("# DO NOT EDIT! This file will be overwritten by %1.").arg(generator),
        QStringLiteral("# Any customization should be done in ~/.gtkrc-2.0.mine instead."),
        QString()};
    l << QStringLiteral("include \"%1/.gtkrc-2.0.mine\"").arg(home)
      << QStringLiteral("gtk-theme-name=") + q(s.gtkTheme)
      << QStringLiteral("gtk-icon-theme-name=") + q(s.iconTheme)
      << QStringLiteral("gtk-font-name=") + q(s.fontName)
      << QStringLiteral("gtk-cursor-theme-name=") + q(s.cursorTheme)
      << QStringLiteral("gtk-cursor-theme-size=") + QString::number(s.cursorSize)
      << QStringLiteral("gtk-toolbar-style=") + s.iniToolbarStyle
      << QStringLiteral("gtk-toolbar-icon-size=") + s.iniToolbarIconSize
      << QStringLiteral("gtk-button-images=") + one(s.buttonImages)
      << QStringLiteral("gtk-menu-images=") + one(s.menuImages)
      << QStringLiteral("gtk-enable-event-sounds=") + one(s.eventSounds)
      << QStringLiteral("gtk-enable-input-feedback-sounds=") + one(s.inputFeedbackSounds)
      << QStringLiteral("gtk-xft-antialias=") + one(s.fontAntialiasing != QLatin1String("none"))
      << QStringLiteral("gtk-xft-hinting=") + one(s.fontHinting != QLatin1String("none"))
      << QStringLiteral("gtk-xft-hintstyle=") + q(hintStyle(s.fontHinting))
      << QStringLiteral("gtk-xft-rgba=") + q(s.fontRgbaOrder);
    return l;
}

QStringList xsettingsd(const ThemeState &s)
{
    const auto q = [](const QString &v) { return QLatin1Char('"') + v + QLatin1Char('"'); };
    return {QStringLiteral("Net/ThemeName ") + q(s.gtkTheme),
            QStringLiteral("Net/IconThemeName ") + q(s.iconTheme),
            QStringLiteral("Gtk/CursorThemeName ") + q(s.cursorTheme),
            QStringLiteral("Net/EnableEventSounds ") + one(s.eventSounds),
            QStringLiteral("Net/EnableInputFeedbackSounds ") + one(s.inputFeedbackSounds),
            QStringLiteral("Xft/Antialias ") + one(s.fontAntialiasing != QLatin1String("none")),
            QStringLiteral("Xft/Hinting ") + one(s.fontHinting != QLatin1String("none")),
            QStringLiteral("Xft/HintStyle ") + q(hintStyle(s.fontHinting)),
            QStringLiteral("Xft/RGBA ") + q(s.fontRgbaOrder)};
}

QStringList indexTheme(const QString &cursorTheme, const QString &generator)
{
    QStringList l{QStringLiteral("# This file is written by %1. Do not edit.").arg(generator),
                  QStringLiteral("[Icon Theme]"), QStringLiteral("Name=Default"),
                  QStringLiteral("Comment=Default Cursor Theme")};
    if (cursorTheme != QLatin1String("default"))
        l << QStringLiteral("Inherits=") + cursorTheme;
    return l;
}

QString gtkrc2Path()
{
    const QString rc = qEnvironmentVariable("GTK2_RC_FILES");
    return rc.isEmpty() ? KwePaths::home() + QStringLiteral("/.gtkrc-2.0") : rc;
}

QString iconsFolderForIndexTheme()
{
    const QString legacy = KwePaths::home() + QStringLiteral("/.icons");
    if (QFileInfo::exists(legacy))
        return legacy;
    const QString xdg = KwePaths::dataHome() + QStringLiteral("/icons");
    return QFileInfo::exists(xdg) ? xdg : QString();
}

StepResult writeSettingsIni(const ThemeState &s, const QStringList &preservedLines)
{
    QString err;
    const QString p3 = cfg(QStringLiteral("gtk-3.0/settings.ini"));
    if (!IniFile::writeTextAtomic(p3, joinLines(settingsIni3(s, preservedLines)), &err))
        return StepResult::fail(err);
    const QString p4 = cfg(QStringLiteral("gtk-4.0/settings.ini"));
    if (!IniFile::writeTextAtomic(p4, joinLines(settingsIni4(s)), &err))
        return StepResult::fail(err);
    return StepResult::ok(QStringLiteral("gtk-3.0/settings.ini, gtk-4.0/settings.ini"));
}

StepResult writeGtkrc2(const ThemeState &s)
{
    QString err;
    const QString path = gtkrc2Path();
    if (!IniFile::writeTextAtomic(
            path, joinLines(gtkrc2(s, KwePaths::home(), QStringLiteral("lgl-kwe-look"))), &err))
        return StepResult::fail(err);
    return StepResult::ok(path);
}

StepResult writeXsettingsd(const ThemeState &s)
{
    QString err;
    const QString path = cfg(QStringLiteral("xsettingsd/xsettingsd.conf"));
    if (!IniFile::writeTextAtomic(path, joinLines(xsettingsd(s)), &err))
        return StepResult::fail(err);
    return StepResult::ok(path);
}

StepResult writeIndexTheme(const ThemeState &s)
{
    const QString folder = iconsFolderForIndexTheme();
    if (folder.isEmpty())
        return StepResult::warn(QStringLiteral("no icons folder found, index.theme not written"));

    QString err;
    const QString path = folder + QStringLiteral("/default/index.theme");
    if (!IniFile::writeTextAtomic(
            path, joinLines(indexTheme(s.cursorTheme, QStringLiteral("lgl-kwe-look"))), &err))
        return StepResult::fail(err);
    return StepResult::ok(path);
}

StepResult clearGtk4Symlinks()
{
    QStringList removed;
    for (const char *rel : {"gtk-4.0/gtk.css", "gtk-4.0/gtk-dark.css", "gtk-4.0/assets", "assets"}) {
        const QString p = cfg(QLatin1String(rel));
        // Regular files and directories are never removed: they are the user's
        // (or Noctalia's), not ours.
        if (isThemeLink(p) && QFile::remove(p))
            removed << QLatin1String(rel);
    }
    return StepResult::ok(removed.isEmpty() ? QStringLiteral("no theme symlinks to remove")
                                            : QStringLiteral("removed ") + removed.join(QStringLiteral(", ")));
}

StepResult linkGtk4(const ThemeState &s, const QString &themePath)
{
    if (s.gtkTheme.isEmpty() || themePath.isEmpty())
        return StepResult::warn(QStringLiteral("GTK theme path unknown, GTK4 links skipped"));
    if (!QFileInfo(themePath + QStringLiteral("/gtk-4.0")).isDir())
        return StepResult::warn(QStringLiteral("%1 has no gtk-4.0 directory").arg(s.gtkTheme));

    clearGtk4Symlinks();

    struct Item { QString src; QString dst; };
    const QList<Item> items{
        {themePath + QStringLiteral("/gtk-4.0/gtk.css"),      cfg(QStringLiteral("gtk-4.0/gtk.css"))},
        {themePath + QStringLiteral("/gtk-4.0/gtk-dark.css"), cfg(QStringLiteral("gtk-4.0/gtk-dark.css"))},
        {themePath + QStringLiteral("/gtk-4.0/assets"),       cfg(QStringLiteral("gtk-4.0/assets"))},
        {themePath + QStringLiteral("/assets"),               cfg(QStringLiteral("assets"))}};

    QDir().mkpath(cfg(QStringLiteral("gtk-4.0")));

    QStringList linked, skipped;
    for (const Item &it : items) {
        if (!QFileInfo::exists(it.src))
            continue;
        if (QFileInfo::exists(it.dst) || QFileInfo(it.dst).isSymLink()) {
            skipped << QFileInfo(it.dst).fileName();   // a regular file or foreign link: leave it
            continue;
        }
        if (QFile::link(it.src, it.dst))
            linked << QFileInfo(it.dst).fileName();
        else
            return StepResult::fail(QStringLiteral("cannot link %1").arg(it.dst));
    }

    QString detail = QStringLiteral("linked: %1").arg(linked.isEmpty() ? QStringLiteral("none")
                                                                       : linked.join(QStringLiteral(", ")));
    if (!skipped.isEmpty()) {
        return StepResult::warn(detail + QStringLiteral("; left untouched (already exist): ")
                                + skipped.join(QStringLiteral(", ")));
    }
    return StepResult::ok(detail);
}

}  // namespace GtkExport
