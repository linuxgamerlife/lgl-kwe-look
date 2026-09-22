#pragma once
#include <QStringList>

#include "preferences.h"

// One-time import of nwg-look's state:
//   ~/.config/nwg-look/config              (JSON export preferences)
//   ~/.local/share/nwg-look/gsettings      (gsettings backup, same format as ours)
namespace Migration {

bool needed();

struct Report {
    bool ran = false;
    QStringList notes;
};

// Imports into `prefs` (in memory) and saves them. Never overwrites an existing
// lgl-kwe-look gsettings backup. Marks the migration done so it runs once.
Report run(Preferences *prefs);

// Pure JSON -> Preferences mapping, exposed for tests. Returns the keys it applied.
QStringList applyNwgLookJson(const QByteArray &json, Preferences *prefs, bool importGtk4Symlinks);

}  // namespace Migration
