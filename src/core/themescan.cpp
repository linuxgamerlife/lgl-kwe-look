#include "themescan.h"

#include "inifile.h"
#include "kwepaths.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <algorithm>
#include <cstdlib>

namespace ThemeScan {

namespace {

QStringList existingSubdirs(const QStringList &dataDirs, const QString &home, const QString &sub,
                            const QString &homeLegacy)
{
    QStringList dirs;
    for (const QString &d : dataDirs) {
        const QString p = d + QLatin1Char('/') + sub;
        if (QFileInfo(p).isDir())
            dirs << p;
    }
    if (!home.isEmpty() && !homeLegacy.isEmpty()) {
        const QString p = home + QLatin1Char('/') + homeLegacy;
        if (QFileInfo(p).isDir() && !dirs.contains(p))
            dirs << p;
    }
    return dirs;
}

// Directory entries, following symlinks like Go's os.Stat did.
QStringList childDirs(const QString &dir)
{
    QStringList out;
    const QDir d(dir);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        out << fi.fileName();
    }
    return out;
}

QStringList splitList(const QString &v)
{
    QStringList out;
    for (const QString &p : v.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString t = p.trimmed();
        if (!t.isEmpty())
            out << t;
    }
    return out;
}

const QStringList &excludedIconFolders()
{
    static const QStringList ex{QStringLiteral("default"), QStringLiteral("hicolor"),
                                QStringLiteral("locolor")};
    return ex;
}

}  // namespace

IndexTheme readIndexTheme(const QString &themeDir)
{
    IndexTheme t;
    QString text;
    if (!IniFile::readText(themeDir + QStringLiteral("/index.theme"), &text))
        return t;
    t.valid = true;
    t.name = IniFile::readValue(text, QStringLiteral("Icon Theme"), QStringLiteral("Name"));
    t.comment = IniFile::readValue(text, QStringLiteral("Icon Theme"), QStringLiteral("Comment"));
    t.directories = splitList(
        IniFile::readValue(text, QStringLiteral("Icon Theme"), QStringLiteral("Directories")));
    t.inherits = splitList(
        IniFile::readValue(text, QStringLiteral("Icon Theme"), QStringLiteral("Inherits")));
    t.hasDirectories = !t.directories.isEmpty();
    return t;
}

QList<GtkTheme> gtkThemes(const QStringList &dataDirs, const QString &home)
{
    static const QStringList exclusions{QStringLiteral("Default"), QStringLiteral("Emacs")};

    QList<GtkTheme> out;
    QSet<QString> seen;
    for (const QString &root : existingSubdirs(dataDirs, home, QStringLiteral("themes"),
                                               QStringLiteral(".themes"))) {
        for (const QString &name : childDirs(root)) {
            if (seen.contains(name) || exclusions.contains(name))
                continue;
            const QString themePath = root + QLatin1Char('/') + name;
            const QStringList inner = childDirs(themePath);
            const bool hasGtk = std::any_of(inner.cbegin(), inner.cend(), [](const QString &s) {
                return s.startsWith(QLatin1String("gtk-"));
            });
            if (!hasGtk)
                continue;
            seen.insert(name);
            out.append({name, themePath});
        }
    }
    std::sort(out.begin(), out.end(),
              [](const GtkTheme &a, const GtkTheme &b) { return a.name < b.name; });
    return out;
}

QList<IconTheme> iconThemes(const QStringList &dataDirs, const QString &home)
{
    QList<IconTheme> out;
    QSet<QString> seen;
    for (const QString &root : existingSubdirs(dataDirs, home, QStringLiteral("icons"),
                                               QStringLiteral(".icons"))) {
        for (const QString &folder : childDirs(root)) {
            if (excludedIconFolders().contains(folder) || seen.contains(folder))
                continue;
            const QString path = root + QLatin1Char('/') + folder;
            const IndexTheme idx = readIndexTheme(path);
            if (!idx.valid || !idx.hasDirectories)
                continue;
            seen.insert(folder);
            out.append({folder, idx.name.isEmpty() ? folder : idx.name, path, idx.inherits});
        }
    }
    std::sort(out.begin(), out.end(), [](const IconTheme &a, const IconTheme &b) {
        return a.displayName.toUpper() < b.displayName.toUpper();
    });
    return out;
}

QList<CursorTheme> cursorThemes(const QStringList &dataDirs, const QString &home)
{
    QList<CursorTheme> out;
    QSet<QString> seen;
    for (const QString &root : existingSubdirs(dataDirs, home, QStringLiteral("icons"),
                                               QStringLiteral(".icons"))) {
        for (const QString &folder : childDirs(root)) {
            if (excludedIconFolders().contains(folder) || seen.contains(folder))
                continue;
            const QString path = root + QLatin1Char('/') + folder;
            if (!QFileInfo(path + QStringLiteral("/cursors")).isDir())
                continue;
            seen.insert(folder);
            const IndexTheme idx = readIndexTheme(path);
            out.append({folder, idx.name.isEmpty() ? folder : idx.name, path});
        }
    }
    std::sort(out.begin(), out.end(), [](const CursorTheme &a, const CursorTheme &b) {
        return a.folder.toUpper() < b.folder.toUpper();
    });
    return out;
}

QList<GtkTheme> gtkThemes()      { return gtkThemes(KwePaths::dataDirs(), KwePaths::home()); }
QList<IconTheme> iconThemes()    { return iconThemes(KwePaths::dataDirs(), KwePaths::home()); }
QList<CursorTheme> cursorThemes(){ return cursorThemes(KwePaths::dataDirs(), KwePaths::home()); }

QString findIconFile(const QString &themeDir, const QString &iconName, int size)
{
    const IndexTheme idx = readIndexTheme(themeDir);
    if (!idx.valid)
        return QString();

    QString text;
    IniFile::readText(themeDir + QStringLiteral("/index.theme"), &text);

    QString best;
    int bestScore = 1 << 30;
    for (const QString &dir : idx.directories) {
        const QString base = themeDir + QLatin1Char('/') + dir + QLatin1Char('/') + iconName;
        QString found;
        for (const char *ext : {".svg", ".png"}) {
            if (QFileInfo::exists(base + QLatin1String(ext))) {
                found = base + QLatin1String(ext);
                break;
            }
        }
        if (found.isEmpty())
            continue;

        const bool scalable = IniFile::readValue(text, dir, QStringLiteral("Type"))
                                  .compare(QLatin1String("Scalable"), Qt::CaseInsensitive) == 0;
        const int dirSize = IniFile::readValue(text, dir, QStringLiteral("Size")).toInt();
        // Scalable icons render at any size; otherwise pick the closest nominal size.
        const int score = scalable ? 0 : std::abs(dirSize - size) + 1;
        if (score < bestScore) {
            bestScore = score;
            best = found;
        }
    }
    return best;
}

}  // namespace ThemeScan
