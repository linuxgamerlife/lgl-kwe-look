#pragma once
#include <QColor>
#include <QFont>
#include <QList>
#include <QPalette>
#include <QString>
#include <QStringList>

#include "inifile.h"
#include "stepresult.h"

// Unified qt5ct / qt6ct configuration API.
//
// The vendored qt5ct and qt6ct pages are structurally identical, so one page set
// takes a QtTarget. This app only edits qt5ct.conf / qt6ct.conf; the runtime
// plugins (libqt5ct.so, libqt6ct.so) come from Fedora's qt5ct and qt6ct packages.
enum class QtTarget { Qt5, Qt6 };

namespace QtCt {

QString toolName(QtTarget t);      // "qt5ct" / "qt6ct"
QString majorName(QtTarget t);     // "Qt5" / "Qt6"

QString configDir(QtTarget t);
QString configFile(QtTarget t);
QString userColorSchemesDir(QtTarget t);
QString userStyleSheetsDir(QtTarget t);
QString styleColorsFile(QtTarget t);
QStringList sharedColorSchemeDirs(QtTarget t);
QStringList sharedStyleSheetDirs(QtTarget t);

// QPalette::NColorRoles for that Qt major: 21 for Qt5, 22 for Qt6 (adds Accent).
int paletteRoleCount(QtTarget t);

// ---- plugin discovery (Qt5 is edited from a Qt6 process, so no QStyleFactory) ----
QString pluginRoot(QtTarget t);
QStringList availableStyles(QtTarget t);
QStringList availablePlatformThemes(QtTarget t);
// Filename -> factory key heuristics, exposed for tests.
QString styleKeyFromFileName(const QString &fileName);
QString platformThemeKeyFromFileName(const QString &fileName);

// ---- colour schemes ----
struct Scheme {
    QList<QColor> active;
    QList<QColor> inactive;
    QList<QColor> disabled;

    int size() const { return int(active.size()); }
    bool isValid() const { return size() > 0; }
};

// Accepts 20 (pre-PlaceholderText), 21 or 22 entries per group, like the plugins.
// A KDE colour scheme (.colors, KColorScheme) is converted with the usual KColorScheme to
// QPalette mapping; the Inactive group equals Active and Disabled is a faded Active, so the
// result is approximate.
bool loadScheme(const QString &path, Scheme *out);
// Pads with derived colours or truncates so the scheme has exactly `roleCount` roles.
// Padding: PlaceholderText (20) = Text with alpha 128, Accent (21) = Highlight.
Scheme adaptRoleCount(const Scheme &s, int roleCount);
Scheme fromPalette(const QPalette &p, int roleCount);
QPalette toPalette(const Scheme &s, const QPalette &base);
QString schemeToText(const Scheme &s);
bool saveScheme(const QString &path, const Scheme &s, QString *error);

struct SchemeEntry {
    QString name;
    QString path;
    bool writable = false;
    bool managed = false;   // owned by the session (Noctalia); never edited or removed
    bool kde = false;       // KDE colour scheme (.colors): read-only, listed for Qt6 only
};
QList<SchemeEntry> findSchemes(QtTarget t);
// noctalia.conf and noctalia.colors are rewritten by the shell on every wallpaper change.
bool isManagedSchemePath(const QString &path);
// KDE colour schemes live in <datadir>/color-schemes/*.colors.
QStringList kdeColorSchemeDirs();
bool isKdeSchemePath(const QString &path);
QString kdeSchemeName(const QString &path);   // [General] Name, else the file name
QString resolvePath(const QString &path);   // expands ~ and $VAR/

// ---- config writes ----
// Read-modify-write of only the given keys, atomic, no QSettings held open.
StepResult applyUpdates(QtTarget t, const QList<IniFile::Update> &updates, const QString &what);

// ---- fonts ----
// qt5ct.conf and qt6ct.conf store QFont::toString(), and the format differs: Qt5 has 10
// fields with a 0-99 weight (Normal = 50), Qt6 has 16 fields with a 1-1000 weight
// (Normal = 400). Qt6's QFont::fromString reads the old format, Qt5's does not read the
// new one, so the string is always produced in the format of the target.
QString fontToConfigString(const QFont &font, QtTarget target);
int qt6WeightToQt5(int weight);

// Convenience encoders shared by the pages.
IniFile::Update setString(const QString &section, const QString &key, const QString &value);
IniFile::Update setBool(const QString &section, const QString &key, bool value);
IniFile::Update setInt(const QString &section, const QString &key, int value);
IniFile::Update setList(const QString &section, const QString &key, const QStringList &value);

}  // namespace QtCt
