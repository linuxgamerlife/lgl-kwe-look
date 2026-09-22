// SPDX-License-Identifier: MPL-2.0
#pragma once
#include <QString>

#include "stepresult.h"

// Flatpak integration: environment overrides plus a port of stylepak, which
// copies the current GTK theme into ~/.themes and gives Flatpak read access.
//
// The stylepak part is a C++ translation of nwg-look's Go port of stylepak
// (https://github.com/refi64/stylepak, MPL-2.0). This file therefore stays under
// the MPL-2.0 (see LICENSES/MPL-2.0-stylepak.txt).
namespace Flatpak {

bool available();

// `flatpak override --user --env NAME=value` / `--unset-env NAME`
StepResult overrideEnv(const QString &name, const QString &value);
StepResult unsetEnv(const QString &name);

// Theme names accepted by installUserTheme: letters, digits, '.', '_' and '-'.
bool validThemeName(const QString &theme);

// Copies the gtk-* directories of `theme` into ~/.themes/<theme> (marked so a
// later run may update or remove it) and grants Flatpak read access to ~/.themes.
// A pre-existing ~/.themes/<theme> that we did not create is never modified.
StepResult installUserTheme(const QString &theme);

}  // namespace Flatpak
