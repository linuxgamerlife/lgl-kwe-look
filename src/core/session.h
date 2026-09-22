#pragma once
#include <QList>
#include <QString>

#include "preferences.h"
#include "qtct.h"

// Which desktop session the app is running in, and what an Apply writes for it.
//
// KineticWE is the primary target. Other desktops are detected so the app can say so and only
// write the files that make sense there (for example kcminputrc on Plasma, or the qt5ct and
// qt6ct icon mirrors only where those tools are in use).
namespace Session {

enum class Type { KineticWE, Kde, Gnome, Xfce, Cinnamon, Mate, Lxqt, Budgie, Other };

struct Info {
    Type type = Type::Other;
    QString name;      // for display: "KineticWE", "KDE", "GNOME", ...
    QString desktop;   // XDG_CURRENT_DESKTOP as set, empty when unset
    QString display;   // "Wayland", "X11" or empty
};

// Reads the environment on every call, nothing is cached (the tests change it).
Info detect();
// XDG_CURRENT_DESKTOP style value ("ubuntu:GNOME"), or a DESKTOP_SESSION name ("plasma").
Type typeFromDesktop(const QString &value);

// What an Apply writes here, beyond the GTK and qt5ct/qt6ct files every session gets.
bool writesKdeGlobals();      // KineticWE settings present, a Plasma session, or kdeglobals exists
bool writesKcminputrc();      // Plasma reads the cursor theme from kcminputrc
bool mirrorsQtCt(QtTarget t); // icon_theme mirrored into qt5ct.conf / qt6ct.conf

// One place an Apply writes to, for the Overview page.
struct Output {
    enum class Group { Desktop, Gtk, Qt };
    Group group = Group::Desktop;
    QString what;
    QString path;          // a file, or a description when isFile is false
    bool exists = false;
    bool isFile = true;
};
// The outputs for the current session and the given export preferences.
QList<Output> outputs(const Preferences &prefs);

}  // namespace Session
