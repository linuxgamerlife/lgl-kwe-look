#include <QtTest>

#include "core/applypipeline.h"
#include "core/inifile.h"
#include "core/qtct.h"
#include "core/session.h"
#include "core/sessionsinks.h"
#include "core/themestate.h"
#include "testenv.h"

// The sink set and its ordering, run against a temporary HOME. gsettings and the session
// bus are made unreachable so the test can never change the developer's real settings: those
// steps are expected to fail here and are not asserted on.
class TstSinks : public QObject
{
    Q_OBJECT

    QTemporaryDir m_emptyBin;

    void isolate()
    {
        qputenv("PATH", m_emptyBin.path().toUtf8());
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/lgl-kwe-look-test");
        // Whether qt5ct/qt6ct are "in use" must come from the test's own files, not this machine.
        qunsetenv("QT_QPA_PLATFORMTHEME");
    }

private slots:
    void initTestCase() { isolate(); }

    // ---- session detection and the per-session sink set ----

    void sessionTypeFromDesktop()
    {
        using T = Session::Type;
        QVERIFY(Session::typeFromDesktop("KineticWE") == T::KineticWE);
        QVERIFY(Session::typeFromDesktop("KDE") == T::Kde);
        QVERIFY(Session::typeFromDesktop("plasma") == T::Kde);
        QVERIFY(Session::typeFromDesktop("ubuntu:GNOME") == T::Gnome);
        QVERIFY(Session::typeFromDesktop("Budgie:GNOME") == T::Budgie);   // not GNOME
        QVERIFY(Session::typeFromDesktop("X-Cinnamon") == T::Cinnamon);
        QVERIFY(Session::typeFromDesktop("XFCE") == T::Xfce);
        QVERIFY(Session::typeFromDesktop("sway") == T::Other);
        QVERIFY(Session::typeFromDesktop("") == T::Other);
    }

    void kdeSessionWritesKdeFilesAndNoKineticWeFiles()
    {
        TestEnv env;
        qputenv("XDG_CURRENT_DESKTOP", "KDE");

        ThemeState base, now;
        now.iconTheme = "Papirus";
        now.cursorTheme = "Bibata";
        now.cursorSize = 32;

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);

        QCOMPARE(IniFile::readFileValue(env.path(".config/kdeglobals"), "Icons", "Theme"), QString("Papirus"));
        QCOMPARE(IniFile::readFileValue(env.path(".config/kdeglobals"), "KDE", "cursorTheme"), QString("Bibata"));
        QCOMPARE(IniFile::readFileValue(env.path(".config/kcminputrc"), "Mouse", "cursorTheme"), QString("Bibata"));
        QCOMPARE(IniFile::readFileValue(env.path(".config/kcminputrc"), "Mouse", "cursorSize"), QString("32"));
        QVERIFY(!QFileInfo::exists(env.path(".config/kineticwe")));
        // Plasma has its own Qt platform theme: qt5ct and qt6ct are not set up behind its back.
        QVERIFY(!QFileInfo::exists(QtCt::configFile(QtTarget::Qt5)));
        QVERIFY(!QFileInfo::exists(QtCt::configFile(QtTarget::Qt6)));
        qunsetenv("XDG_CURRENT_DESKTOP");
    }

    void gnomeSessionLeavesKdeFilesAlone()
    {
        TestEnv env;
        qputenv("XDG_CURRENT_DESKTOP", "GNOME");

        ThemeState base, now;
        now.iconTheme = "Papirus";
        now.cursorTheme = "Bibata";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);

        QVERIFY(!QFileInfo::exists(env.path(".config/kdeglobals")));
        QVERIFY(!QFileInfo::exists(env.path(".config/kcminputrc")));
        QVERIFY(!QFileInfo::exists(QtCt::configFile(QtTarget::Qt6)));
        qunsetenv("XDG_CURRENT_DESKTOP");
    }

    void outputsFollowTheSession()
    {
        TestEnv env;
        qputenv("XDG_CURRENT_DESKTOP", "KDE");
        const auto has = [](const QList<Session::Output> &all, const QString &path) {
            for (const Session::Output &o : all) {
                if (o.path == path)
                    return true;
            }
            return false;
        };

        Preferences prefs;
        QList<Session::Output> kde = Session::outputs(prefs);
        QVERIFY(has(kde, env.path(".config/kdeglobals")));
        QVERIFY(has(kde, env.path(".config/kcminputrc")));
        QVERIFY(!has(kde, env.path(".config/kineticwe/appearance.kwe")));

        QDir().mkpath(env.path(".config/kineticwe"));
        qputenv("XDG_CURRENT_DESKTOP", "KineticWE");
        const QList<Session::Output> kwe = Session::outputs(prefs);
        QVERIFY(has(kwe, env.path(".config/kineticwe/appearance.kwe")));
        QVERIFY(!has(kwe, env.path(".config/kcminputrc")));
        qunsetenv("XDG_CURRENT_DESKTOP");
    }

    void iconChoiceReachesEverySink()
    {
        TestEnv env;
        QDir().mkpath(env.path(".config/kineticwe"));
        TestEnv::write(env.path(".config/kdeglobals"), "[Icons]\nTheme=breeze\n");
        TestEnv::write(env.path(".config/qt5ct/qt5ct.conf"), "[Appearance]\nicon_theme=breeze\nstyle=Fusion\n");
        TestEnv::write(env.path(".config/qt6ct/qt6ct.conf"), "[Appearance]\nicon_theme=breeze\nstyle=Fusion\n");

        ThemeState base;
        base.iconTheme = "breeze";
        ThemeState now = base;
        now.iconTheme = "Papirus";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);

        const QString app = env.path(".config/kineticwe/appearance.kwe");
        QCOMPARE(IniFile::readFileValue(app, "Appearance", "IconTheme"), QString("Papirus"));
        QCOMPARE(IniFile::readFileValue(env.path(".config/kdeglobals"), "Icons", "Theme"), QString("Papirus"));
        for (const char *t : {"qt5ct", "qt6ct"}) {
            const QString f = env.path(QString(".config/%1/%1.conf").arg(t));
            QCOMPARE(IniFile::readFileValue(f, "Appearance", "icon_theme"), QString("Papirus"));
            QCOMPARE(IniFile::readFileValue(f, "Appearance", "style"), QString("Fusion"));   // untouched
        }
    }

    void loginSyncDoesNotRevertTheChoice()
    {
        // start-kineticwe.sh step 4h copies appearance.kwe [Appearance] IconTheme into qt*ct
        // icon_theme. If what we wrote there already equals it, the sync is a no-op.
        TestEnv env;
        QDir().mkpath(env.path(".config/kineticwe"));
        ThemeState base, now;
        base.iconTheme = "breeze";
        now.iconTheme = "Papirus";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);

        const QString fromSession = IniFile::readFileValue(env.path(".config/kineticwe/appearance.kwe"),
                                                           "Appearance", "IconTheme");
        for (QtTarget t : {QtTarget::Qt5, QtTarget::Qt6})
            QCOMPARE(IniFile::readFileValue(QtCt::configFile(t), "Appearance", "icon_theme"), fromSession);
    }

    void cursorChoiceReachesTheCompositorConfig()
    {
        TestEnv env;
        QDir().mkpath(env.path(".config/kineticwe"));
        TestEnv::write(env.path(".config/kdeglobals"), "[KDE]\ncursorTheme=breeze_cursors\n");

        ThemeState base, now;
        now.cursorTheme = "Bibata";
        now.cursorSize = 36;

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);

        const QString comp = env.path(".config/kineticwe/kineticwe.kwe");
        QCOMPARE(IniFile::readFileValue(comp, "Mouse", "cursorTheme"), QString("Bibata"));
        QCOMPARE(IniFile::readFileValue(comp, "Mouse", "cursorSize"), QString("36"));
        const QString app = env.path(".config/kineticwe/appearance.kwe");
        QCOMPARE(IniFile::readFileValue(app, "Appearance", "CursorTheme"), QString("Bibata"));
        QCOMPARE(IniFile::readFileValue(app, "Appearance", "CursorSize"), QString("36"));
        QCOMPARE(IniFile::readFileValue(env.path(".config/kdeglobals"), "KDE", "cursorTheme"), QString("Bibata"));
    }

    void applyIsIdempotent()
    {
        TestEnv env;
        QDir().mkpath(env.path(".config/kineticwe"));
        ThemeState base, now;
        now.iconTheme = "Papirus";
        now.cursorTheme = "Bibata";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);
        const QString a = TestEnv::read(env.path(".config/kineticwe/appearance.kwe"));
        const QString q = TestEnv::read(QtCt::configFile(QtTarget::Qt6));
        ApplyPipeline::run(plan);
        QCOMPARE(TestEnv::read(env.path(".config/kineticwe/appearance.kwe")), a);
        QCOMPARE(TestEnv::read(QtCt::configFile(QtTarget::Qt6)), q);
    }

    void kweFilesAreNotCreatedOutsideAKweSession()
    {
        TestEnv env;   // no ~/.config/kineticwe
        ThemeState base, now;
        now.iconTheme = "Papirus";
        now.cursorTheme = "Bibata";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        ApplyPipeline::run(plan);
        QVERIFY(!QFileInfo::exists(env.path(".config/kineticwe")));
    }

    void stepsRunInSinkOrder()
    {
        TestEnv env;
        QDir().mkpath(env.path(".config/kineticwe"));
        TestEnv::write(env.path(".config/kdeglobals"), "");
        ThemeState base, now;
        now.iconTheme = "Papirus";

        ApplyPlan plan;
        Sinks::addSessionSteps(plan, now, base, false);
        const QList<ApplyStep> steps = plan.sorted();
        QStringList labels;
        for (const ApplyStep &s : steps)
            labels << s.label;

        const auto idx = [&](const QString &needle) {
            for (int i = 0; i < labels.size(); ++i) {
                if (labels.at(i).contains(needle))
                    return i;
            }
            return -1;
        };
        QVERIFY(idx("appearance.kwe") >= 0);
        QVERIFY(idx("appearance.kwe") < idx("kdeglobals"));
        QVERIFY(idx("kdeglobals") < idx("icon_theme"));
        QVERIFY(idx("icon_theme") < idx("gsettings"));
        QVERIFY(idx("gsettings") < idx("notify"));
    }

    void unchangedStateProducesNoSteps()
    {
        ThemeState s;
        ApplyPlan plan;
        Sinks::addSessionSteps(plan, s, s, false);
        QVERIFY(plan.isEmpty());
        Sinks::addSessionSteps(plan, s, s, true);   // force = re-apply everything
        QVERIFY(!plan.isEmpty());
    }

    void stepFailureDoesNotStopTheOthers()
    {
        ApplyPlan plan;
        int ran = 0;
        plan.add(20, "b", [&]() { ++ran; return StepResult::ok(); });
        plan.add(10, "a", [&]() { ++ran; return StepResult::fail("boom"); });
        plan.add(30, "c", [&]() { ++ran; return StepResult::warn("hm"); });
        const ApplyReport r = ApplyPipeline::run(plan);
        QCOMPARE(ran, 3);
        QCOMPARE(r.lines.first().label, QString("a"));
        QVERIFY(r.hasFailures());
        QVERIFY(r.hasWarnings());
    }

    void equalOrderKeepsInsertionOrder()
    {
        ApplyPlan plan;
        for (const char *n : {"x", "y", "z"})
            plan.add(5, n, []() { return StepResult::ok(); });
        const QList<ApplyStep> s = plan.sorted();
        QCOMPARE(s.at(0).label, QString("x"));
        QCOMPARE(s.at(2).label, QString("z"));
    }

    // ---- GTK4 export must never disturb Noctalia's files ----

    void gtkApplyLeavesNoctaliaFilesAlone_data()
    {
        QTest::addColumn<bool>("gtk4Export");
        QTest::newRow("gtk4 export off (KWE default)") << false;
        QTest::newRow("gtk4 export on") << true;
    }
    void gtkApplyLeavesNoctaliaFilesAlone()
    {
        QFETCH(bool, gtk4Export);
        TestEnv env;
        QDir().mkpath(env.path("usr/share/themes/T/gtk-4.0"));
        TestEnv::write(env.path("usr/share/themes/T/gtk-4.0/gtk.css"), "theme");

        const QString imp = "@import 'noctalia.css';\n";
        const QStringList guarded{".config/gtk-3.0/gtk.css", ".config/gtk-4.0/gtk.css",
                                  ".config/gtk-4.0/noctalia.css", ".config/qt6ct/colors/noctalia.conf"};
        for (const QString &g : guarded)
            TestEnv::write(env.path(g), g.contains("noctalia.") ? "colors" : imp);

        Sinks::GtkContext ctx;
        ctx.state.gtkTheme = "T";
        ctx.state.iconTheme = "Papirus";
        ctx.prefs.exportGtk4Symlinks = gtk4Export;
        ctx.prefsBaseline = ctx.prefs;
        ctx.baseline = ThemeState();
        ctx.gtkThemePath = env.path("usr/share/themes/T");
        ctx.force = true;

        QMap<QString, QByteArray> before;
        for (const QString &g : guarded) {
            QFile f(env.path(g));
            QVERIFY(f.open(QIODevice::ReadOnly));
            before[g] = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256);
        }

        ApplyPlan plan;
        Sinks::addGtkSteps(plan, ctx);
        ApplyPipeline::run(plan);

        for (const QString &g : guarded) {
            QVERIFY2(!QFileInfo(env.path(g)).isSymLink(), qPrintable(g));
            QFile f(env.path(g));
            QVERIFY(f.open(QIODevice::ReadOnly));
            QCOMPARE(QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256), before.value(g));
        }
        // and the export itself did run
        QVERIFY(QFileInfo::exists(env.path(".config/gtk-3.0/settings.ini")));
    }

    void exportOnlySkipsGsettings()
    {
        Sinks::GtkContext ctx;
        ctx.exportOnly = true;
        ApplyPlan plan;
        Sinks::addGtkSteps(plan, ctx);
        for (const ApplyStep &s : plan.sorted())
            QVERIFY2(s.order != ApplyOrder::GSettings, qPrintable(s.label));
        QVERIFY(!plan.isEmpty());
    }
};

QTEST_MAIN(TstSinks)
#include "tst_sinks.moc"
