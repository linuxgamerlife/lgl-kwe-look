#include <QtTest>

#include "core/inifile.h"
#include "testenv.h"

using IniFile::Update;

// Parity with the session's Python patcher (start-kineticwe.sh step 4h), plus the
// QSettings-compatible value encoding.
class TstIniFile : public QObject
{
    Q_OBJECT
private slots:
    void replacesExistingKey()
    {
        const QString in = "[Appearance]\nicon_theme=old\nstyle=Fusion\n";
        bool changed = false;
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "breeze")}, &changed);
        QVERIFY(changed);
        QCOMPARE(out, QString("[Appearance]\nicon_theme=breeze\nstyle=Fusion\n"));
    }

    void unchangedValueIsNotAChange()
    {
        const QString in = "[Appearance]\nicon_theme=breeze\n";
        bool changed = true;
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "breeze")}, &changed);
        QVERIFY(!changed);
        QCOMPARE(out, in);
    }

    void insertsMissingKeyBeforeTrailingBlankLines()
    {
        // Python parity: the section's trailing blank lines stay where they are.
        const QString in = "[Appearance]\nstyle=Fusion\n\n[Fonts]\ngeneral=x\n";
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "breeze")});
        QCOMPARE(out, QString("[Appearance]\nstyle=Fusion\nicon_theme=breeze\n\n[Fonts]\ngeneral=x\n"));
    }

    void appendsMissingSectionAfterBlankLine()
    {
        const QString in = "[Fonts]\ngeneral=x\n";
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "breeze")});
        QCOMPARE(out, QString("[Fonts]\ngeneral=x\n\n[Appearance]\nicon_theme=breeze\n"));
    }

    void createsFromEmpty()
    {
        const QString out = IniFile::patchText(QString(), {Update::set("Icons", "Theme", "breeze")});
        QCOMPARE(out, QString("[Icons]\nTheme=breeze\n"));
    }

    void keepsCommentsAndUnknownKeys()
    {
        const QString in = "# hand written\n[Appearance]\n; note\ncustom=1\nicon_theme=old\n";
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "x")});
        QCOMPARE(out, QString("# hand written\n[Appearance]\n; note\ncustom=1\nicon_theme=x\n"));
    }

    void onlyTouchesTheNamedSection()
    {
        const QString in = "[Other]\nicon_theme=keep\n[Appearance]\nicon_theme=old\n";
        const QString out = IniFile::patchText(in, {Update::set("Appearance", "icon_theme", "new")});
        QCOMPARE(out, QString("[Other]\nicon_theme=keep\n[Appearance]\nicon_theme=new\n"));
    }

    void removesKey()
    {
        const QString in = "[Mouse]\ncursorTheme=a\ncursorSize=24\n";
        const QString out = IniFile::patchText(in, {Update::erase("Mouse", "cursorTheme")});
        QCOMPARE(out, QString("[Mouse]\ncursorSize=24\n"));
    }

    void handlesCrLf()
    {
        const QString out = IniFile::patchText("[A]\r\nk=1\r\n", {Update::set("A", "k", "2")});
        QCOMPARE(out, QString("[A]\nk=2\n"));
    }

    void readValueDecodes()
    {
        const QString text = "[Fonts]\ngeneral=\"Noto Sans,11,-1,5\"\n";
        QCOMPARE(IniFile::readValue(text, "Fonts", "general"), QString("Noto Sans,11,-1,5"));
        QCOMPARE(IniFile::readValue(text, "Fonts", "missing", "dflt"), QString("dflt"));
    }

    void encodeRoundTrip_data()
    {
        QTest::addColumn<QString>("value");
        QTest::newRow("plain") << QString("breeze");
        QTest::newRow("comma") << QString("Noto Sans,11,-1,5,400");
        QTest::newRow("quote") << QString("say \"hi\"");
        QTest::newRow("backslash") << QString("a\\b");
        QTest::newRow("equals") << QString("a=b");
        QTest::newRow("edge space") << QString(" padded ");
    }
    void encodeRoundTrip()
    {
        QFETCH(QString, value);
        QCOMPARE(IniFile::decodeValue(IniFile::encodeValue(value)), value);
    }

    void encodedFontIsQuotedLikeQSettings()
    {
        // Unquoted, QSettings would split the font at the commas and toString() would yield "".
        QCOMPARE(IniFile::encodeValue("Noto Sans,11"), QString("\"Noto Sans,11\""));
    }

    void emptyListMatchesQSettings()
    {
        // The plugin checks childKeys().contains("gui_effects"): an explicit empty list
        // must stay present, not disappear.
        QCOMPARE(IniFile::encodeList({}), QString("@Invalid()"));
        QCOMPARE(IniFile::encodeList({"a", "b"}), QString("a, b"));
    }

    void listReadBackByQSettings()
    {
        TestEnv env;
        const QString file = env.path("l.conf");
        QVERIFY(IniFile::applyUpdates(file, {Update::set("Interface", "stylesheets",
                                                          IniFile::encodeList({"/a/b.qss", "/c d/e.qss"}))},
                                      nullptr));
        QSettings s(file, QSettings::IniFormat);
        QCOMPARE(s.value("Interface/stylesheets").toStringList(), QStringList({"/a/b.qss", "/c d/e.qss"}));
    }

    void fontReadBackByQSettings()
    {
        TestEnv env;
        const QString file = env.path("f.conf");
        const QString font = "Noto Sans,11,-1,5,400,0,0,0,0,0,0,0,0,0,0,1";
        QVERIFY(IniFile::applyUpdates(file, {Update::set("Fonts", "general", IniFile::encodeValue(font))}, nullptr));
        QSettings s(file, QSettings::IniFormat);
        QCOMPARE(s.value("Fonts/general").toString(), font);
    }

    void applyUpdatesSkipsWriteWhenNothingChanges()
    {
        TestEnv env;
        const QString file = env.path("c.conf");
        QVERIFY(IniFile::applyUpdates(file, {Update::set("A", "k", "1")}, nullptr));
        const QDateTime before = QFileInfo(file).lastModified();
        QTest::qSleep(1100);
        bool changed = true;
        QVERIFY(IniFile::applyUpdates(file, {Update::set("A", "k", "1")}, nullptr, &changed));
        QVERIFY(!changed);
        QCOMPARE(QFileInfo(file).lastModified(), before);
    }

    void writesThroughSymlink()
    {
        // The KWE root's kdeglobals is a symlink into the real config home.
        TestEnv env;
        const QString real = env.path("real/kdeglobals");
        const QString link = env.path("root/kdeglobals");
        TestEnv::write(real, "[Icons]\nTheme=old\n");
        QDir().mkpath(env.path("root"));
        QVERIFY(QFile::link(real, link));
        QVERIFY(IniFile::applyUpdates(link, {Update::set("Icons", "Theme", "breeze")}, nullptr));
        QVERIFY(QFileInfo(link).isSymLink());
        QCOMPARE(TestEnv::read(real), QString("[Icons]\nTheme=breeze\n"));
    }

    void preservesPermissions()
    {
        TestEnv env;
        const QString file = env.path("p.conf");
        TestEnv::write(file, "[A]\nk=1\n");
        QVERIFY(QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        QVERIFY(IniFile::applyUpdates(file, {Update::set("A", "k", "2")}, nullptr));
        QCOMPARE(QFileInfo(file).permissions(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    void refusesMultilineValues()
    {
        TestEnv env;
        QString err;
        QVERIFY(!IniFile::applyUpdates(env.path("x.conf"), {Update::set("A", "k", "a\nb=c")}, &err));
        QVERIFY(!err.isEmpty());
        QVERIFY(!QFileInfo::exists(env.path("x.conf")));
    }
};

QTEST_MAIN(TstIniFile)
#include "tst_inifile.moc"
