#include <QtTest>

#include "core/qtct.h"
#include "testenv.h"

class TstQtCt : public QObject
{
    Q_OBJECT

    static QtCt::Scheme scheme(int n)
    {
        QtCt::Scheme s;
        for (int i = 0; i < n; ++i) {
            s.active << QColor(i, 0, 0, 255);
            s.inactive << QColor(0, i, 0, 255);
            s.disabled << QColor(0, 0, i, 255);
        }
        return s;
    }

private slots:
    void pathsFollowTheTarget()
    {
        TestEnv env;
        QCOMPARE(QtCt::configFile(QtTarget::Qt5), env.path(".config/qt5ct/qt5ct.conf"));
        QCOMPARE(QtCt::configFile(QtTarget::Qt6), env.path(".config/qt6ct/qt6ct.conf"));
        QCOMPARE(QtCt::userColorSchemesDir(QtTarget::Qt6), env.path(".config/qt6ct/colors"));
        QCOMPARE(QtCt::styleColorsFile(QtTarget::Qt5), env.path(".config/qt5ct/style-colors.conf"));
    }

    void roleCountsDifferByMajor()
    {
        QCOMPARE(QtCt::paletteRoleCount(QtTarget::Qt5), 21);
        QCOMPARE(QtCt::paletteRoleCount(QtTarget::Qt6), 22);
    }

    // ---- palette role adaptation: 21 (Qt5) <-> 22 (Qt6, adds Accent) ----

    void padsAccentFromHighlight()
    {
        const QtCt::Scheme in = scheme(21);
        const QtCt::Scheme out = QtCt::adaptRoleCount(in, 22);
        QCOMPARE(out.size(), 22);
        QCOMPARE(out.active.at(21), in.active.at(12));      // Accent = Highlight
        QCOMPARE(out.inactive.at(21), in.inactive.at(12));
        QCOMPARE(out.disabled.at(21), in.disabled.at(12));
        QCOMPARE(out.active.at(5), in.active.at(5));        // the rest untouched
    }

    void padsPlaceholderFromTextForTheOldFormat()
    {
        const QtCt::Scheme out = QtCt::adaptRoleCount(scheme(20), 22);
        QCOMPARE(out.size(), 22);
        QCOMPARE(out.active.at(20).red(), 6);               // Text
        QCOMPARE(out.active.at(20).alpha(), 128);
        QCOMPARE(out.active.at(21), out.active.at(12));
    }

    void truncatesForQt5()
    {
        const QtCt::Scheme in = scheme(22);
        const QtCt::Scheme out = QtCt::adaptRoleCount(in, 21);
        QCOMPARE(out.size(), 21);
        QCOMPARE(out.active.last(), in.active.at(20));
    }

    void adaptingToTheSameCountIsANoOp()
    {
        const QtCt::Scheme in = scheme(22);
        const QtCt::Scheme out = QtCt::adaptRoleCount(in, 22);
        QCOMPARE(out.active, in.active);
    }

    void schemeFileRoundTripsThroughQSettings()
    {
        // The plugins read these files with QSettings, so that is what we load with too.
        TestEnv env;
        const QString path = env.path("s.conf");
        QVERIFY(QtCt::saveScheme(path, scheme(22), nullptr));
        QtCt::Scheme back;
        QVERIFY(QtCt::loadScheme(path, &back));
        QCOMPARE(back.size(), 22);
        QCOMPARE(back.active.at(7), QColor(7, 0, 0, 255));
        QCOMPARE(back.disabled.at(20), QColor(0, 0, 20, 255));
    }

    void savedSchemeUsesArgbHexLikeTheVendoredFiles()
    {
        QtCt::Scheme s = scheme(21);
        s.active[0] = QColor(0x12, 0x34, 0x56, 0x78);
        QVERIFY(QtCt::schemeToText(s).contains("active_colors=#78123456,"));
    }

    void loadsA21RoleQt5FileAndA22RoleFileAlike()
    {
        TestEnv env;
        QVERIFY(QtCt::saveScheme(env.path("q5.conf"), scheme(21), nullptr));
        QVERIFY(QtCt::saveScheme(env.path("q6.conf"), scheme(22), nullptr));
        QtCt::Scheme a, b;
        QVERIFY(QtCt::loadScheme(env.path("q5.conf"), &a));
        QVERIFY(QtCt::loadScheme(env.path("q6.conf"), &b));
        QCOMPARE(a.size(), 21);
        QCOMPARE(b.size(), 22);
    }

    void rejectsATooShortScheme()
    {
        TestEnv env;
        QtCt::Scheme s;
        QVERIFY(!QtCt::loadScheme(env.path("missing.conf"), &s));
        TestEnv::write(env.path("short.conf"), "[ColorScheme]\nactive_colors=#ff000000\n");
        QVERIFY(!QtCt::loadScheme(env.path("short.conf"), &s));
    }

    void paletteConversionKeepsColours()
    {
        QPalette p;
        p.setColor(QPalette::Active, QPalette::Highlight, QColor("#0986d3"));
        const QtCt::Scheme s = QtCt::fromPalette(p, 22);
        QCOMPARE(s.size(), 22);
        QCOMPARE(s.active.at(QPalette::Highlight), QColor("#0986d3"));
        QCOMPARE(QtCt::toPalette(s, QPalette()).color(QPalette::Active, QPalette::Highlight), QColor("#0986d3"));
    }

    // ---- plugin discovery ----

    void styleKeysFromFileNames()
    {
        QCOMPARE(QtCt::styleKeyFromFileName("breeze5.so"), QString("breeze"));
        QCOMPARE(QtCt::styleKeyFromFileName("breeze6.so"), QString("breeze"));
        QCOMPARE(QtCt::styleKeyFromFileName("libqt5ct-style.so"), QString("qt5ct-style"));
    }

    void platformThemeKeysFromFileNames()
    {
        QCOMPARE(QtCt::platformThemeKeyFromFileName("libqgtk3.so"), QString("gtk3"));
        QCOMPARE(QtCt::platformThemeKeyFromFileName("libqxdgdesktopportal.so"), QString("xdgdesktopportal"));
        QCOMPARE(QtCt::platformThemeKeyFromFileName("KDEPlasmaPlatformTheme5.so"), QString("kde"));
        QCOMPARE(QtCt::platformThemeKeyFromFileName("libqt5ct.so"), QString("qt5ct"));
    }

    // ---- managed (Noctalia) schemes ----

    void noctaliaSchemeIsManagedAndReadOnly()
    {
        TestEnv env;
        QVERIFY(QtCt::saveScheme(env.path(".config/qt6ct/colors/noctalia.conf"), scheme(22), nullptr));
        QVERIFY(QtCt::saveScheme(env.path(".config/qt6ct/colors/mine.conf"), scheme(22), nullptr));

        bool sawNoctalia = false;
        bool sawMine = false;
        for (const QtCt::SchemeEntry &e : QtCt::findSchemes(QtTarget::Qt6)) {
            if (e.name == "noctalia") {
                sawNoctalia = true;
                QVERIFY(e.managed);
                QVERIFY(!e.writable);
            } else if (e.name == "mine") {
                sawMine = true;
                QVERIFY(!e.managed);
                QVERIFY(e.writable);
            }
        }
        QVERIFY(sawNoctalia);
        QVERIFY(sawMine);
        QVERIFY(QtCt::isManagedSchemePath("/home/u/.local/share/color-schemes/noctalia.colors"));
        QVERIFY(!QtCt::isManagedSchemePath("/home/u/.config/qt6ct/colors/airy.conf"));
    }

    // ---- KDE colour schemes (.colors) ----

    void kdeColorSchemeIsListedAndConverted()
    {
        TestEnv env;
        const QString path = env.path(".local/share/color-schemes/demo.colors");
        TestEnv::write(path, QStringLiteral("[General]\nName=Demo Scheme\n\n"
                                            "[Colors:Window]\nBackgroundNormal=26,17,15\nForegroundNormal=241,223,217\n\n"
                                            "[Colors:View]\nBackgroundNormal=10,20,30\nForegroundNormal=200,210,220\n\n"
                                            "[Colors:Selection]\nBackgroundNormal=114,53,31\nForegroundNormal=255,219,207\n"));
        TestEnv::write(env.path(".local/share/color-schemes/empty.colors"), QStringLiteral("[General]\nName=Empty\n"));

        QVERIFY(QtCt::isKdeSchemePath(path));
        QCOMPARE(QtCt::kdeSchemeName(path), QString("Demo Scheme"));

        QtCt::Scheme s;
        QVERIFY(QtCt::loadScheme(path, &s));
        QCOMPARE(s.size(), 22);
        QCOMPARE(s.active.at(QPalette::Window), QColor(26, 17, 15));
        QCOMPARE(s.active.at(QPalette::Base), QColor(10, 20, 30));
        QCOMPARE(s.active.at(QPalette::Text), QColor(200, 210, 220));
        QCOMPARE(s.active.at(QPalette::Highlight), QColor(114, 53, 31));
        QCOMPARE(s.inactive.at(QPalette::Window), s.active.at(QPalette::Window));
        QVERIFY(s.disabled.at(QPalette::Text) != s.active.at(QPalette::Text));

        // Without a Window and a View colour it is not a KColorScheme.
        QtCt::Scheme none;
        QVERIFY(!QtCt::loadScheme(env.path(".local/share/color-schemes/empty.colors"), &none));

        bool sawQt6 = false;
        for (const QtCt::SchemeEntry &e : QtCt::findSchemes(QtTarget::Qt6)) {
            if (e.path == path) {
                sawQt6 = true;
                QVERIFY(e.kde);
                QVERIFY(!e.writable);
                QCOMPARE(e.name, QString("Demo Scheme"));
            }
        }
        QVERIFY(sawQt6);
        for (const QtCt::SchemeEntry &e : QtCt::findSchemes(QtTarget::Qt5))
            QVERIFY(!e.kde);   // qt5ct does not read KColorScheme files
    }

    // ---- fonts: the two majors use different QFont string formats ----

    void qt5FontStringUsesTheOldFormat()
    {
        QFont f("Noto Sans", 11);
        f.setWeight(QFont::Normal);
        QCOMPARE(QtCt::fontToConfigString(f, QtTarget::Qt5), QString("Noto Sans,11,-1,5,50,0,0,0,0,0"));

        f.setWeight(QFont::Bold);
        f.setItalic(true);
        QCOMPARE(QtCt::fontToConfigString(f, QtTarget::Qt5), QString("Noto Sans,11,-1,5,75,1,0,0,0,0"));
    }

    void qt6FontStringIsQFontToString()
    {
        const QFont f("Noto Sans", 11);
        QCOMPARE(QtCt::fontToConfigString(f, QtTarget::Qt6), f.toString());
    }

    void qt5WeightScale()
    {
        QCOMPARE(QtCt::qt6WeightToQt5(100), 0);
        QCOMPARE(QtCt::qt6WeightToQt5(400), 50);
        QCOMPARE(QtCt::qt6WeightToQt5(500), 57);
        QCOMPARE(QtCt::qt6WeightToQt5(700), 75);
        QCOMPARE(QtCt::qt6WeightToQt5(900), 87);
    }

    void qt6ReadsBothFontFormats()
    {
        QFont a;
        QVERIFY(a.fromString("Noto Sans,11,-1,5,75,1,0,0,0,0"));   // what we write for Qt5
        QCOMPARE(a.family(), QString("Noto Sans"));
        QVERIFY(a.bold());
        QVERIFY(a.italic());
    }

    // ---- config writes ----

    void applyUpdatesWritesOnlyTheGivenKeys()
    {
        TestEnv env;
        TestEnv::write(QtCt::configFile(QtTarget::Qt6),
                       "[Appearance]\nstyle=Breeze\nicon_theme=old\n\n[Interface]\nwheel_scroll_lines=5\n");

        const StepResult r = QtCt::applyUpdates(
            QtTarget::Qt6, {QtCt::setString("Appearance", "icon_theme", "breeze")}, "icon theme");
        QVERIFY(r.isOk());

        QCOMPARE(TestEnv::read(QtCt::configFile(QtTarget::Qt6)),
                 QString("[Appearance]\nstyle=Breeze\nicon_theme=breeze\n\n[Interface]\nwheel_scroll_lines=5\n"));
    }

    void writtenValuesReadBackThroughQSettingsLikeThePluginDoes()
    {
        TestEnv env;
        const QFont f("Noto Sans", 11);
        const StepResult r = QtCt::applyUpdates(
            QtTarget::Qt6,
            {QtCt::setString("Appearance", "style", "Fusion"),
             QtCt::setBool("Appearance", "custom_palette", true),
             QtCt::setString("Fonts", "general", QtCt::fontToConfigString(f, QtTarget::Qt6)),
             QtCt::setInt("Interface", "double_click_interval", 400),
             QtCt::setList("Interface", "stylesheets", {"/a/x.qss", "/b/y.qss"}),
             QtCt::setList("Interface", "gui_effects", {})},
            "all");
        QVERIFY(r.isOk());

        QSettings s(QtCt::configFile(QtTarget::Qt6), QSettings::IniFormat);
        QCOMPARE(s.value("Appearance/style").toString(), QString("Fusion"));
        QVERIFY(s.value("Appearance/custom_palette").toBool());
        QCOMPARE(s.value("Fonts/general").toString(), f.toString());
        QCOMPARE(s.value("Interface/double_click_interval").toInt(), 400);
        QCOMPARE(s.value("Interface/stylesheets").toStringList(), QStringList({"/a/x.qss", "/b/y.qss"}));
        // an explicit empty list stays a present key: "no effects", not "defaults"
        s.beginGroup("Interface");
        QVERIFY(s.childKeys().contains("gui_effects"));
        QVERIFY(s.value("gui_effects").toStringList().isEmpty());
    }

    void resolvesHomeAndVariables()
    {
        TestEnv env;
        QCOMPARE(QtCt::resolvePath("~/x"), env.path("x"));
        qputenv("LGLKWE_TEST_DIR", "/opt/t");
        QCOMPARE(QtCt::resolvePath("$LGLKWE_TEST_DIR/x"), QString("/opt/t/x"));
    }
};

QTEST_MAIN(TstQtCt)
#include "tst_qtct.moc"
