#include <QtTest>

#include "core/themescan.h"
#include "testenv.h"

class TstThemeScan : public QObject
{
    Q_OBJECT

    static void icon(const TestEnv &env, const QString &folder, const QString &name, const QString &dirs,
                     const QString &extra = QString())
    {
        TestEnv::write(env.path("usr/share/icons/" + folder + "/index.theme"),
                       "[Icon Theme]\nName=" + name + "\n" + dirs + "\n" + extra);
    }

private slots:
    void gtkThemesNeedAGtkDirectory()
    {
        TestEnv env;
        QDir().mkpath(env.path("usr/share/themes/Good/gtk-3.0"));
        QDir().mkpath(env.path("usr/share/themes/NoGtk/metacity-1"));
        QDir().mkpath(env.path("usr/share/themes/Default/gtk-3.0"));   // excluded like nwg-look
        QDir().mkpath(env.path("usr/share/themes/Emacs/gtk-3.0"));
        QDir().mkpath(env.path(".themes/Home/gtk-4.0"));

        const auto themes = ThemeScan::gtkThemes({env.path("usr/share")}, env.root());
        QStringList names;
        for (const auto &t : themes)
            names << t.name;
        QCOMPARE(names, QStringList({"Good", "Home"}));
    }

    void iconThemesNeedDirectoriesAndSkipHicolor()
    {
        TestEnv env;
        icon(env, "breeze", "Breeze", "Directories=48x48/apps,scalable/apps");
        icon(env, "hicolor", "Hicolor", "Directories=48x48/apps");
        icon(env, "cursoronly", "Cursor Only", "");   // no Directories=
        icon(env, "Adwaita", "Adwaita", "Directories=48x48/apps", "Inherits=hicolor\n");

        const auto themes = ThemeScan::iconThemes({env.path("usr/share")}, env.root());
        QStringList folders;
        for (const auto &t : themes)
            folders << t.folder;
        QCOMPARE(folders, QStringList({"Adwaita", "breeze"}));   // sorted by display name, case-insensitive
        QCOMPARE(themes.first().inherits, QStringList({"hicolor"}));
    }

    void cursorThemesNeedACursorsDirectory()
    {
        TestEnv env;
        icon(env, "breeze_cursors", "Breeze", "Directories=");
        QDir().mkpath(env.path("usr/share/icons/breeze_cursors/cursors"));
        icon(env, "plain", "Plain", "Directories=48x48/apps");

        const auto themes = ThemeScan::cursorThemes({env.path("usr/share")}, env.root());
        QCOMPARE(themes.size(), 1);
        QCOMPARE(themes.first().folder, QString("breeze_cursors"));
        QCOMPARE(themes.first().displayName, QString("Breeze"));
    }

    void findsIconPreferringClosestSize()
    {
        TestEnv env;
        const QString root = env.path("usr/share/icons/t");
        TestEnv::write(root + "/index.theme",
                       "[Icon Theme]\nName=T\nDirectories=16x16/places,48x48/places,64x64/places\n"
                       "[16x16/places]\nSize=16\n[48x48/places]\nSize=48\n[64x64/places]\nSize=64\n");
        for (const char *d : {"16x16", "48x48", "64x64"})
            TestEnv::write(root + "/" + d + "/places/folder.png", "x");

        QVERIFY(ThemeScan::findIconFile(root, "folder", 48).contains("48x48"));
        QVERIFY(ThemeScan::findIconFile(root, "folder", 60).contains("64x64"));
        QVERIFY(ThemeScan::findIconFile(root, "missing", 48).isEmpty());
    }

    void scalableWinsOverFixedSizes()
    {
        TestEnv env;
        const QString root = env.path("usr/share/icons/s");
        TestEnv::write(root + "/index.theme",
                       "[Icon Theme]\nName=S\nDirectories=48x48/places,scalable/places\n"
                       "[48x48/places]\nSize=48\n[scalable/places]\nSize=48\nType=Scalable\n");
        TestEnv::write(root + "/48x48/places/folder.png", "x");
        TestEnv::write(root + "/scalable/places/folder.svg", "<svg/>");
        QVERIFY(ThemeScan::findIconFile(root, "folder", 48).endsWith("folder.svg"));
    }
};

QTEST_MAIN(TstThemeScan)
#include "tst_themescan.moc"
