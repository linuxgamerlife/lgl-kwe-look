#include "cli.h"

#include "applypipeline.h"
#include "kwepaths.h"
#include "migration.h"
#include "preferences.h"
#include "sessionsinks.h"
#include "themescan.h"
#include "themestate.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

namespace Cli {

namespace {

QString themePathFor(const QString &name)
{
    for (const ThemeScan::GtkTheme &t : ThemeScan::gtkThemes()) {
        if (t.name == name)
            return t.path;
    }
    return QString();
}

int runPlan(const ApplyPlan &plan)
{
    QTextStream out(stdout);
    const ApplyReport report = ApplyPipeline::run(plan);
    for (const ApplyLine &l : report.lines) {
        const char *tag = l.result.status == StepResult::Ok        ? "ok  "
                        : l.result.status == StepResult::Warning   ? "warn"
                                                                    : "FAIL";
        out << '[' << tag << "] " << l.label;
        if (!l.result.detail.isEmpty())
            out << ": " << l.result.detail;
        out << '\n';
    }
    return report.hasFailures() ? 1 : 0;
}

}  // namespace

bool wantsCli(const QStringList &args)
{
    static const QStringList flags{QStringLiteral("-a"), QStringLiteral("--apply"),
                                   QStringLiteral("-x"), QStringLiteral("--export"),
                                   QStringLiteral("-r"), QStringLiteral("--restore-defaults"),
                                   QStringLiteral("-v"), QStringLiteral("--version"),
                                   QStringLiteral("-h"), QStringLiteral("--help")};
    for (const QString &a : args) {
        if (flags.contains(a))
            return true;
    }
    return false;
}

int run(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("lgl-kwe-look"));
    QCoreApplication::setApplicationVersion(QStringLiteral(LGLKWE_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("LinuxGamerLife"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("LGL KWE Look: GTK, Qt5 and Qt6 appearance for KineticWE and other desktops"));
    parser.addHelpOption();
    parser.addOptions({
        {{QStringLiteral("v"), QStringLiteral("version")}, QStringLiteral("Display version information")},
        {{QStringLiteral("a"), QStringLiteral("apply")}, QStringLiteral("Apply the stored gsettings backup and quit")},
        {{QStringLiteral("x"), QStringLiteral("export")}, QStringLiteral("Export the GTK config files and quit")},
        {{QStringLiteral("r"), QStringLiteral("restore-defaults")}, QStringLiteral("Restore default GTK settings and quit")},
        {{QStringLiteral("y"), QStringLiteral("yes")}, QStringLiteral("Do not ask for confirmation (with -r)")},
    });
    parser.process(app);

    QTextStream out(stdout);
    if (parser.isSet(QStringLiteral("version"))) {
        out << "lgl-kwe-look version " << LGLKWE_VERSION << '\n';
        return 0;
    }

    Preferences prefs = Prefs::load();
    if (Migration::needed())
        Migration::run(&prefs);

    QStringList preserved;
    const ThemeState current = ThemeStateIO::load(&preserved);

    Sinks::GtkContext ctx;
    ctx.prefs = prefs;
    ctx.prefsBaseline = prefs;
    ctx.preservedIniLines = preserved;

    ApplyPlan plan;

    if (parser.isSet(QStringLiteral("restore-defaults"))) {
        if (!parser.isSet(QStringLiteral("yes"))) {
            out << "Restore default GTK settings? y/N " << Qt::flush;
            QTextStream in(stdin);
            if (in.readLine().trimmed().toUpper() != QLatin1String("Y"))
                return 0;
        }
        // GTK-only values are reset. The icon and cursor choice belongs to the
        // session and is left as it is.
        ThemeState defaults;
        defaults.iconTheme = current.iconTheme;
        defaults.cursorTheme = current.cursorTheme;
        defaults.cursorSize = current.cursorSize;

        ctx.state = defaults;
        ctx.baseline = current;
        ctx.force = true;
        ctx.gtkThemePath = themePathFor(defaults.gtkTheme);
        Sinks::addGtkSteps(plan, ctx);
        return runPlan(plan);
    }

    if (parser.isSet(QStringLiteral("apply"))) {
        ThemeState stored = current;
        if (!ThemeStateIO::loadBackup(ThemeStateIO::backupPath(), &stored)) {
            out << "No stored settings found: " << ThemeStateIO::backupPath() << '\n';
            return 1;
        }
        // In a KineticWE session appearance.kwe owns the icon and cursor theme.
        if (KwePaths::kweConfigPresent())
            stored = ThemeStateIO::readSessionFiles(stored);
        Sinks::addGSettingsSteps(plan, stored, current, true, true);
        int rc = runPlan(plan);

        if (parser.isSet(QStringLiteral("export"))) {
            ApplyPlan exportPlan;
            ctx.state = stored;
            ctx.baseline = stored;
            ctx.exportOnly = true;
            ctx.gtkThemePath = themePathFor(stored.gtkTheme);
            Sinks::addGtkSteps(exportPlan, ctx);
            rc |= runPlan(exportPlan);
        }
        return rc;
    }

    if (parser.isSet(QStringLiteral("export"))) {
        ctx.state = current;
        ctx.baseline = current;
        ctx.exportOnly = true;
        ctx.gtkThemePath = themePathFor(current.gtkTheme);
        Sinks::addGtkSteps(plan, ctx);
        return runPlan(plan);
    }

    return 0;
}

}  // namespace Cli
