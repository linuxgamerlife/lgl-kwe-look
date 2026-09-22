#pragma once
#include <QString>
#include <QStringList>

#include "stepresult.h"
#include "themestate.h"

// Export of the GTK-side config files. Port of nwg-look's saveGtkIni3/4,
// saveGtkRc20, saveXsettingsd, saveIndexTheme and linkGtk4Stuff.
//
// The content builders are pure functions so the tests can compare their output
// against nwg-look's. Deliberate deviations from nwg-look:
//  * xsettingsd writes Net/EnableInputFeedbackSounds (nwg-look omits "Net/").
//  * the GTK4 symlink export never deletes or replaces a regular file and only
//    removes symlinks that point into a themes directory. Noctalia keeps regular
//    gtk-3.0/gtk.css and gtk-4.0/gtk.css files (they @import noctalia.css); a
//    blind rm + ln -s would destroy them.
namespace GtkExport {

struct Options {
    bool settingsIni = true;
    bool gtkrc2 = true;
    bool indexTheme = true;
    bool xsettingsd = true;
    bool gtk4Symlinks = false;
};

// ---- pure content builders ----
QStringList settingsIni3(const ThemeState &s, const QStringList &preservedLines);
QStringList settingsIni4(const ThemeState &s);
QStringList gtkrc2(const ThemeState &s, const QString &home,
                   const QString &generator = QStringLiteral("nwg-look"));
QStringList xsettingsd(const ThemeState &s);
QStringList indexTheme(const QString &cursorTheme,
                       const QString &generator = QStringLiteral("nwg-look"));

QString hintStyle(const QString &gsettingsHinting);   // slight -> hintslight, unknown -> hintnone

// ---- writers ----
// Each returns one StepResult; a non-existent target directory is created.
StepResult writeSettingsIni(const ThemeState &s, const QStringList &preservedLines);
StepResult writeGtkrc2(const ThemeState &s);
StepResult writeXsettingsd(const ThemeState &s);
StepResult writeIndexTheme(const ThemeState &s);
StepResult linkGtk4(const ThemeState &s, const QString &themePath);
// Removes only symlinks that point into a themes directory.
StepResult clearGtk4Symlinks();

// The directory index.theme goes to: ~/.icons if it exists, else the XDG icons dir.
QString iconsFolderForIndexTheme();
QString gtkrc2Path();

}  // namespace GtkExport
