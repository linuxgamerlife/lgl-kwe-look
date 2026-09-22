#pragma once
#include <QStringList>

// nwg-look command-line parity, so existing autostart lines keep working:
//   -a / --apply             apply the stored gsettings backup, then quit
//   -x / --export            export the config files from the current settings, then quit
//   -r / --restore-defaults  reset the GTK settings to defaults (asks first), then quit
//   -v / --version
//   -d / --debug
// CLI mode uses QCoreApplication only, so it works without a display.
namespace Cli {

bool wantsCli(const QStringList &args);
int run(int argc, char **argv);

}  // namespace Cli
