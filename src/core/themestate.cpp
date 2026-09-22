#include "themestate.h"

#include "gsettingsclient.h"
#include "inifile.h"
#include "kwepaths.h"
#include "session.h"

#include <QDir>
#include <QFile>
#include <cmath>

bool ThemeState::sessionDiffers(const ThemeState &o) const
{
    return iconTheme != o.iconTheme || cursorTheme != o.cursorTheme || cursorSize != o.cursorSize;
}

bool ThemeState::gtkDiffers(const ThemeState &o) const
{
    return gtkTheme != o.gtkTheme || fontName != o.fontName || toolbarStyle != o.toolbarStyle
        || toolbarIconsSize != o.toolbarIconsSize || fontHinting != o.fontHinting
        || fontAntialiasing != o.fontAntialiasing || fontRgbaOrder != o.fontRgbaOrder
        || std::fabs(textScalingFactor - o.textScalingFactor) > 1e-6 || colorScheme != o.colorScheme
        || eventSounds != o.eventSounds || inputFeedbackSounds != o.inputFeedbackSounds
        || iniToolbarStyle != o.iniToolbarStyle || iniToolbarIconSize != o.iniToolbarIconSize
        || buttonImages != o.buttonImages || menuImages != o.menuImages;
}

bool ThemeState::operator==(const ThemeState &o) const
{
    return !sessionDiffers(o) && !gtkDiffers(o);
}

namespace ThemeStateIO {

namespace {

const char *const kIfaceKeys[] = {"gtk-theme", "icon-theme", "font-name", "cursor-theme",
                                  "cursor-size", "toolbar-style", "toolbar-icons-size",
                                  "font-hinting", "font-antialiasing", "font-rgba-order",
                                  "text-scaling-factor", "color-scheme"};

bool isSupportedIniKey(const QString &line)
{
    static const QStringList supported{
        QStringLiteral("gtk-theme-name"), QStringLiteral("gtk-icon-theme-name"),
        QStringLiteral("gtk-font-name"), QStringLiteral("gtk-cursor-theme-name"),
        QStringLiteral("gtk-cursor-theme-size"), QStringLiteral("gtk-toolbar-style"),
        QStringLiteral("gtk-toolbar-icon-size"), QStringLiteral("gtk-button-images"),
        QStringLiteral("gtk-menu-images"), QStringLiteral("gtk-enable-event-sounds"),
        QStringLiteral("gtk-enable-input-feedback-sounds"), QStringLiteral("gtk-xft-antialias"),
        QStringLiteral("gtk-xft-hinting"), QStringLiteral("gtk-xft-hintstyle"),
        QStringLiteral("gtk-xft-rgba"), QStringLiteral("gtk-application-prefer-dark-theme")};
    for (const QString &s : supported) {
        if (line.startsWith(s))
            return true;
    }
    return false;
}

void assign(ThemeState *s, const QString &key, const QString &value)
{
    if (key == QLatin1String("gtk-theme"))                 s->gtkTheme = value;
    else if (key == QLatin1String("icon-theme"))           s->iconTheme = value;
    else if (key == QLatin1String("font-name"))            s->fontName = value;
    else if (key == QLatin1String("cursor-theme"))         s->cursorTheme = value;
    else if (key == QLatin1String("cursor-size")) {
        bool ok = false;
        const int v = value.toInt(&ok);
        if (ok) s->cursorSize = v;
    } else if (key == QLatin1String("toolbar-style"))      s->toolbarStyle = value;
    else if (key == QLatin1String("toolbar-icons-size"))   s->toolbarIconsSize = value;
    else if (key == QLatin1String("font-hinting"))         s->fontHinting = value;
    else if (key == QLatin1String("font-antialiasing"))    s->fontAntialiasing = value;
    else if (key == QLatin1String("font-rgba-order"))      s->fontRgbaOrder = value;
    else if (key == QLatin1String("text-scaling-factor")) {
        bool ok = false;
        const double v = value.toDouble(&ok);
        if (ok) s->textScalingFactor = v;
    } else if (key == QLatin1String("color-scheme"))       s->colorScheme = value;
    else if (key == QLatin1String("event-sounds"))         s->eventSounds = (value == QLatin1String("true"));
    else if (key == QLatin1String("input-feedback-sounds"))s->inputFeedbackSounds = (value == QLatin1String("true"));
}

}  // namespace

ThemeState readGSettings(ThemeState base)
{
    if (!GSettings::available())
        return base;

    for (const char *k : kIfaceKeys) {
        bool ok = false;
        const QString v = GSettings::get(QLatin1String(GSettings::kInterface), QLatin1String(k), &ok);
        if (ok)
            assign(&base, QLatin1String(k), v);
    }
    for (const char *k : {"event-sounds", "input-feedback-sounds"}) {
        bool ok = false;
        const QString v = GSettings::get(QLatin1String(GSettings::kSound), QLatin1String(k), &ok);
        if (ok)
            assign(&base, QLatin1String(k), v);
    }
    return base;
}

ThemeState readSessionFiles(ThemeState base)
{
    using IniFile::readFileValue;

    const QString app = KwePaths::appearanceKwe();
    const QString comp = KwePaths::kineticweKwe();
    const QString kde = KwePaths::kdeglobals();

    QString v = readFileValue(app, QStringLiteral("Appearance"), QStringLiteral("IconTheme"));
    if (v.isEmpty())
        v = readFileValue(kde, QStringLiteral("Icons"), QStringLiteral("Theme"));
    if (!v.isEmpty())
        base.iconTheme = v;

    v = readFileValue(app, QStringLiteral("Appearance"), QStringLiteral("CursorTheme"));
    if (v.isEmpty())
        v = readFileValue(comp, QStringLiteral("Mouse"), QStringLiteral("cursorTheme"));
    if (v.isEmpty())
        v = readFileValue(kde, QStringLiteral("KDE"), QStringLiteral("cursorTheme"));
    if (v.isEmpty())   // Plasma keeps the pointer theme in kcminputrc
        v = readFileValue(KwePaths::kcminputrc(), QStringLiteral("Mouse"), QStringLiteral("cursorTheme"));
    if (!v.isEmpty())
        base.cursorTheme = v;

    QString sz = readFileValue(app, QStringLiteral("Appearance"), QStringLiteral("CursorSize"));
    if (sz.isEmpty())
        sz = readFileValue(comp, QStringLiteral("Mouse"), QStringLiteral("cursorSize"));
    if (sz.isEmpty())
        sz = readFileValue(kde, QStringLiteral("KDE"), QStringLiteral("cursorSize"));
    if (sz.isEmpty())
        sz = readFileValue(KwePaths::kcminputrc(), QStringLiteral("Mouse"), QStringLiteral("cursorSize"));
    bool ok = false;
    const int n = sz.toInt(&ok);
    if (ok && n > 0)
        base.cursorSize = n;
    return base;
}

void readIniExtras(ThemeState *s, QStringList *originalLines)
{
    if (originalLines)
        originalLines->clear();

    QString text;
    if (!IniFile::readText(KwePaths::realConfigHome() + QStringLiteral("/gtk-3.0/settings.ini"), &text))
        return;

    for (QString line : text.split(QLatin1Char('\n'))) {
        line = line.trimmed();
        // Lines the export does not model are appended back verbatim.
        if (originalLines && !line.startsWith(QLatin1Char('[')) && !line.isEmpty()
            && !isSupportedIniKey(line))
            originalLines->append(line);

        if (line.startsWith(QLatin1Char('[')) || line.startsWith(QLatin1Char('#'))
            || !line.contains(QLatin1Char('=')))
            continue;
        const QString key = line.section(QLatin1Char('='), 0, 0).trimmed();
        const QString val = line.section(QLatin1Char('='), 1).trimmed();
        if (key == QLatin1String("gtk-toolbar-style"))          s->iniToolbarStyle = val;
        else if (key == QLatin1String("gtk-toolbar-icon-size")) s->iniToolbarIconSize = val;
        else if (key == QLatin1String("gtk-button-images"))     s->buttonImages = (val == QLatin1String("1"));
        else if (key == QLatin1String("gtk-menu-images"))       s->menuImages = (val == QLatin1String("1"));
    }
}

ThemeState load(QStringList *originalIniLines)
{
    ThemeState s = readGSettings(ThemeState());
    if (Session::writesKdeGlobals())
        s = readSessionFiles(s);
    readIniExtras(&s, originalIniLines);
    return s;
}

QString backupPath()
{
    return KwePaths::appDataDir() + QStringLiteral("/gsettings");
}

bool saveBackup(const ThemeState &s, QString *error)
{
    const auto b = [](bool v) { return v ? QStringLiteral("true") : QStringLiteral("false"); };
    QStringList lines{QStringLiteral("# Generated by lgl-kwe-look, do not edit this file.")};
    lines << QStringLiteral("gtk-theme=") + s.gtkTheme
          << QStringLiteral("icon-theme=") + s.iconTheme
          << QStringLiteral("font-name=") + s.fontName
          << QStringLiteral("cursor-theme=") + s.cursorTheme
          << QStringLiteral("cursor-size=") + QString::number(s.cursorSize)
          << QStringLiteral("toolbar-style=") + s.toolbarStyle
          << QStringLiteral("toolbar-icons-size=") + s.toolbarIconsSize
          << QStringLiteral("font-hinting=") + s.fontHinting
          << QStringLiteral("font-antialiasing=") + s.fontAntialiasing
          << QStringLiteral("font-rgba-order=") + s.fontRgbaOrder
          << QStringLiteral("text-scaling-factor=") + QString::number(s.textScalingFactor, 'f', 2)
          << QStringLiteral("color-scheme=") + s.colorScheme
          << QStringLiteral("event-sounds=") + b(s.eventSounds)
          << QStringLiteral("input-feedback-sounds=") + b(s.inputFeedbackSounds);
    return IniFile::writeTextAtomic(backupPath(), lines.join(QLatin1Char('\n')) + QLatin1Char('\n'),
                                    error);
}

bool loadBackup(const QString &path, ThemeState *s)
{
    QString text;
    if (!IniFile::readText(path, &text))
        return false;
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (line.startsWith(QLatin1Char('#')))
            continue;
        const QStringList parts = line.split(QLatin1Char('='));
        if (parts.size() == 2)
            assign(s, parts.at(0), parts.at(1));
    }
    return true;
}

}  // namespace ThemeStateIO
