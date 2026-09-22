#pragma once
#include <QList>
#include <QString>

#include "applypipeline.h"
#include "inifile.h"
#include "preferences.h"
#include "qtct.h"
#include "themestate.h"

// Builds the ordered write steps for the parts of the state that reach several
// consumers at once.
//
// In a KineticWE session one icon or cursor choice must reach every sink, or the
// value silently reverts (start-kineticwe.sh step 4h rewrites qt*ct icon_theme
// from appearance.kwe at every login). The sink set mirrors Kinetic Settings'
// Icons and Cursors pages, so both tools stay idempotent with each other:
//
//   appearance.kwe -> kineticwe.kwe [Mouse] -> kdeglobals -> qt5ct.conf + qt6ct.conf
//   -> gsettings -> KGlobalSettings.notifyChange
//
// Other desktops (see session.h) get the subset that applies to them: a Plasma session also
// writes kcminputrc, and a full desktop only mirrors the icon theme into qt5ct/qt6ct when those
// are in use.
namespace Sinks {

struct GtkContext {
    ThemeState state;
    ThemeState baseline;
    Preferences prefs;
    Preferences prefsBaseline;
    QStringList preservedIniLines;
    QString gtkThemePath;     // for the GTK4 symlink export
    bool force = false;       // re-apply everything, not just what changed
    bool exportOnly = false;  // CLI -x: write the export files only (no gsettings, no Flatpak)
};

// KGlobalSettings notifyChange type codes used by Kinetic Settings.
enum KdeChange { IconChanged = 4, CursorChanged = 5 };

void addSessionSteps(ApplyPlan &plan, const ThemeState &state, const ThemeState &baseline,
                     bool force);
void addGtkSteps(ApplyPlan &plan, const GtkContext &ctx);

// Every writable key the `gtk` steps own, used by the CLI to apply a stored backup.
void addGSettingsSteps(ApplyPlan &plan, const ThemeState &state, const ThemeState &baseline,
                       bool force, bool includeSession);

StepResult notifyKGlobalSettings(int type, int arg = 0);

// Writes `updates` to qt5ct.conf and/or qt6ct.conf. `linked` writes both.
void addQtCtSteps(ApplyPlan &plan, QtTarget target, bool linked,
                  const QList<IniFile::Update> &updates, const QString &what);

}  // namespace Sinks
