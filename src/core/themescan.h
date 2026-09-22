#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Theme discovery. Port of nwg-look's getThemeNames / getIconThemeNames /
// getCursorThemes. Each scan has an explicit-roots overload so tests can run
// against a temporary tree; the no-argument form uses the XDG environment.
namespace ThemeScan {

struct GtkTheme {
    QString name;   // directory name, what gsettings stores
    QString path;
};

struct IconTheme {
    QString folder;        // directory name, what gsettings / appearance.kwe store
    QString displayName;   // index.theme Name=
    QString path;
    QStringList inherits;
};

struct CursorTheme {
    QString folder;
    QString displayName;
    QString path;          // the theme directory (its cursors/ dir is inside)
};

QList<GtkTheme> gtkThemes(const QStringList &dataDirs, const QString &home);
QList<IconTheme> iconThemes(const QStringList &dataDirs, const QString &home);
QList<CursorTheme> cursorThemes(const QStringList &dataDirs, const QString &home);

QList<GtkTheme> gtkThemes();
QList<IconTheme> iconThemes();
QList<CursorTheme> cursorThemes();

// index.theme parsing: only the [Icon Theme] section is considered.
struct IndexTheme {
    QString name;
    QString comment;
    bool hasDirectories = false;
    QStringList directories;
    QStringList inherits;
    bool valid = false;
};
IndexTheme readIndexTheme(const QString &themeDir);

// Locates an icon file inside one theme directory (SVG or PNG), preferring the
// size closest to `size`. Empty when the theme has no such icon. Used by the
// icon preview, which cannot use QIcon::fromTheme without switching the app's
// own theme.
QString findIconFile(const QString &themeDir, const QString &iconName, int size);

}  // namespace ThemeScan
