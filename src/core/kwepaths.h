#pragma once
#include <QString>
#include <QStringList>

// Path resolution for everything the app reads or writes.
//
// All functions read the environment on every call (nothing is cached) so the
// test suite can point HOME / XDG_* at a temporary directory.
namespace KwePaths {

QString home();
// The real config home: $XDG_CONFIG_HOME or ~/.config. The KineticWE session
// only redirects XDG_CONFIG_HOME for the compositor process, never for apps.
QString realConfigHome();
// The KineticWE settings root: $KWE_CONFIG_HOME or ~/.config/kineticwe.
QString kweConfigHome();
QString dataHome();
// dataHome() followed by $XDG_DATA_DIRS (default /usr/local/share:/usr/share),
// keeping only directories that exist.
QStringList dataDirs();

QString appearanceKwe();   // <kwe>/appearance.kwe  — source of truth for icon + cursor theme
QString kineticweKwe();    // <kwe>/kineticwe.kwe   — what the compositor reads ([Mouse])
QString kdeglobals();      // <config>/kdeglobals
QString kcminputrc();      // <config>/kcminputrc  — where Plasma keeps the cursor theme

QString appConfigDir();    // ~/.config/lgl-kwe-look  (own window state, preferences)
QString appDataDir();      // ~/.local/share/lgl-kwe-look

bool isKineticWeSession();
// True when the KineticWE settings root exists, i.e. the .kwe sinks are worth writing.
bool kweConfigPresent();

}  // namespace KwePaths
