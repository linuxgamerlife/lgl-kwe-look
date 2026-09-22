#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QLocale>
#include <QTranslator>

#include "core/cli.h"
#include "core/migration.h"
#include "core/preferences.h"
#include "mainwindow.h"

// main.cpp — LGL KWE Look entry point.
//
// The GUI runs as the normal user at all times: every write is under $HOME or goes
// through user-level gsettings, so there is no privileged helper.
//
// QT_QPA_PLATFORMTHEME is deliberately left alone. The benchmark app forces "kde" when
// it is unset, but a KineticWE session has no Plasma platform plugin and exports qt5ct.

int main(int argc, char *argv[])
{
    // nwg-look flags (-a, -x, -r, -v) run without a display and without widgets.
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);
    if (Cli::wantsCli(args))
        return Cli::run(argc, argv);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("LGL KWE Look"));
    QApplication::setApplicationVersion(QStringLiteral(LGLKWE_VERSION));
    QApplication::setOrganizationName(QStringLiteral("LinuxGamerLife"));
    QApplication::setDesktopFileName(QStringLiteral("com.linuxgamerlife.lgl-kwe-look"));

    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("com.linuxgamerlife.lgl-kwe-look"),
                                                 QIcon(QStringLiteral(":/icons/lgl-kwe-look.svg"))));

    // Increase base font size by 2pt for readability.
    QFont appFont = app.font();
    if (appFont.pointSize() > 0) {
        appFont.setPointSize(appFont.pointSize() + 2);
        app.setFont(appFont);
    }

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("lgl-kwe-look"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        app.installTranslator(&translator);

    // One-time import of nwg-look's export preferences and gsettings backup.
    if (Migration::needed()) {
        Preferences prefs = Prefs::load();
        Migration::run(&prefs);
    }

    MainWindow window;
    window.show();
    return app.exec();
}
