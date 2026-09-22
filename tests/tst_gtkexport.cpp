#include <QtTest>

#include "core/gtkexport.h"
#include "testenv.h"

// Expected content is nwg-look 1.1.1's output for the same input, transcribed from
// tools.go (saveGtkIni3/4, saveGtkRc20, saveXsettingsd, saveIndexTheme). The one deliberate
// difference is noted where it occurs.
class TstGtkExport : public QObject
{
    Q_OBJECT

    static ThemeState sample()
    {
        ThemeState s;
        s.gtkTheme = "Adwaita-dark";
        s.iconTheme = "Papirus";
        s.fontName = "Noto Sans 11";
        s.cursorTheme = "Bibata";
        s.cursorSize = 32;
        s.eventSounds = true;
        s.inputFeedbackSounds = false;
        s.fontAntialiasing = "grayscale";
        s.fontHinting = "slight";
        s.fontRgbaOrder = "rgb";
        s.colorScheme = "prefer-dark";
        return s;
    }

    static void makeTheme(const TestEnv &env)
    {
        TestEnv::write(env.path("usr/share/themes/T/gtk-4.0/gtk.css"), "theme css");
        TestEnv::write(env.path("usr/share/themes/T/gtk-4.0/gtk-dark.css"), "theme dark css");
        QDir().mkpath(env.path("usr/share/themes/T/gtk-4.0/assets"));
    }

private slots:
    void settingsIni3MatchesNwgLook()
    {
        const QStringList expected{"[Settings]",
                                   "gtk-theme-name=Adwaita-dark",
                                   "gtk-icon-theme-name=Papirus",
                                   "gtk-font-name=Noto Sans 11",
                                   "gtk-cursor-theme-name=Bibata",
                                   "gtk-cursor-theme-size=32",
                                   "gtk-toolbar-style=GTK_TOOLBAR_ICONS",
                                   "gtk-toolbar-icon-size=GTK_ICON_SIZE_LARGE_TOOLBAR",
                                   "gtk-button-images=0",
                                   "gtk-menu-images=0",
                                   "gtk-enable-event-sounds=1",
                                   "gtk-enable-input-feedback-sounds=0",
                                   "gtk-xft-antialias=1",
                                   "gtk-xft-hinting=1",
                                   "gtk-xft-hintstyle=hintslight",
                                   "gtk-xft-rgba=rgb",
                                   "gtk-application-prefer-dark-theme=1"};
        QCOMPARE(GtkExport::settingsIni3(sample(), {}), expected);
    }

    void settingsIni3AppendsUnsupportedLines()
    {
        const QStringList out = GtkExport::settingsIni3(sample(), {"# my comment", "gtk-enable-animations=0", ""});
        QCOMPARE(out.mid(out.size() - 2), QStringList({"# my comment", "gtk-enable-animations=0"}));
    }

    void settingsIni4MatchesNwgLook()
    {
        const QStringList expected{"[Settings]",
                                   "gtk-theme-name=Adwaita-dark",
                                   "gtk-icon-theme-name=Papirus",
                                   "gtk-font-name=Noto Sans 11",
                                   "gtk-cursor-theme-name=Bibata",
                                   "gtk-cursor-theme-size=32",
                                   "gtk-application-prefer-dark-theme=1"};
        QCOMPARE(GtkExport::settingsIni4(sample()), expected);
    }

    void gtkrc2MatchesNwgLook()
    {
        const QStringList expected{"# DO NOT EDIT! This file will be overwritten by nwg-look.",
                                   "# Any customization should be done in ~/.gtkrc-2.0.mine instead.",
                                   "",
                                   "include \"/home/u/.gtkrc-2.0.mine\"",
                                   "gtk-theme-name=\"Adwaita-dark\"",
                                   "gtk-icon-theme-name=\"Papirus\"",
                                   "gtk-font-name=\"Noto Sans 11\"",
                                   "gtk-cursor-theme-name=\"Bibata\"",
                                   "gtk-cursor-theme-size=32",
                                   "gtk-toolbar-style=GTK_TOOLBAR_ICONS",
                                   "gtk-toolbar-icon-size=GTK_ICON_SIZE_LARGE_TOOLBAR",
                                   "gtk-button-images=0",
                                   "gtk-menu-images=0",
                                   "gtk-enable-event-sounds=1",
                                   "gtk-enable-input-feedback-sounds=0",
                                   "gtk-xft-antialias=1",
                                   "gtk-xft-hinting=1",
                                   "gtk-xft-hintstyle=\"hintslight\"",
                                   "gtk-xft-rgba=\"rgb\""};
        QCOMPARE(GtkExport::gtkrc2(sample(), "/home/u"), expected);
    }

    void xsettingsdMatchesNwgLookExceptTheInputFeedbackKey()
    {
        // nwg-look writes "EnableInputFeedbackSounds" without the Net/ prefix, which xsettingsd
        // does not recognise. We write the real key name.
        const QStringList expected{"Net/ThemeName \"Adwaita-dark\"",
                                   "Net/IconThemeName \"Papirus\"",
                                   "Gtk/CursorThemeName \"Bibata\"",
                                   "Net/EnableEventSounds 1",
                                   "Net/EnableInputFeedbackSounds 0",
                                   "Xft/Antialias 1",
                                   "Xft/Hinting 1",
                                   "Xft/HintStyle \"hintslight\"",
                                   "Xft/RGBA \"rgb\""};
        QCOMPARE(GtkExport::xsettingsd(sample()), expected);
    }

    void indexThemeMatchesNwgLook()
    {
        QCOMPARE(GtkExport::indexTheme("Bibata"),
                 QStringList({"# This file is written by nwg-look. Do not edit.", "[Icon Theme]", "Name=Default",
                              "Comment=Default Cursor Theme", "Inherits=Bibata"}));
        // "in hope to fix #90": no Inherits for the theme literally called "default"
        QVERIFY(!GtkExport::indexTheme("default").join('\n').contains("Inherits"));
    }

    void hintingAndAntialiasMapping()
    {
        QCOMPARE(GtkExport::hintStyle("slight"), QString("hintslight"));
        QCOMPARE(GtkExport::hintStyle("medium"), QString("hintmedium"));
        QCOMPARE(GtkExport::hintStyle("full"), QString("hintfull"));
        QCOMPARE(GtkExport::hintStyle("none"), QString("hintnone"));
        QCOMPARE(GtkExport::hintStyle("anything else"), QString("hintnone"));

        ThemeState s = sample();
        s.fontAntialiasing = "none";
        s.fontHinting = "none";
        const QStringList ini = GtkExport::settingsIni3(s, {});
        QVERIFY(ini.contains("gtk-xft-antialias=0"));
        QVERIFY(ini.contains("gtk-xft-hinting=0"));
    }

    void writersCreateTheFiles()
    {
        TestEnv env;
        QVERIFY(GtkExport::writeSettingsIni(sample(), {"gtk-enable-animations=0"}).isOk());
        const QString ini3 = TestEnv::read(env.path(".config/gtk-3.0/settings.ini"));
        QVERIFY(ini3.startsWith("[Settings]\n"));
        QVERIFY(ini3.contains("gtk-enable-animations=0"));
        QVERIFY(QFileInfo::exists(env.path(".config/gtk-4.0/settings.ini")));

        QVERIFY(GtkExport::writeGtkrc2(sample()).isOk());
        QVERIFY(TestEnv::read(env.path(".gtkrc-2.0")).contains("lgl-kwe-look"));

        QVERIFY(GtkExport::writeXsettingsd(sample()).isOk());
        QVERIFY(QFileInfo::exists(env.path(".config/xsettingsd/xsettingsd.conf")));
    }

    void gtkrcHonoursGtk2RcFiles()
    {
        TestEnv env;
        qputenv("GTK2_RC_FILES", env.path("custom/rc").toUtf8());
        QVERIFY(GtkExport::writeGtkrc2(sample()).isOk());
        QVERIFY(QFileInfo::exists(env.path("custom/rc")));
        qunsetenv("GTK2_RC_FILES");
    }

    void indexThemeNeedsAnIconsFolder()
    {
        TestEnv env;
        QCOMPARE(GtkExport::writeIndexTheme(sample()).status, StepResult::Warning);
        QVERIFY(!QFileInfo::exists(env.path(".icons/default/index.theme")));

        QDir().mkpath(env.path(".icons"));
        QVERIFY(GtkExport::writeIndexTheme(sample()).isOk());
        QVERIFY(TestEnv::read(env.path(".icons/default/index.theme")).contains("Inherits=Bibata"));
    }

    // ---- GTK4 symlink policy (Noctalia keeps regular gtk.css files) ----

    void linksGtk4WhenNothingIsInTheWay()
    {
        TestEnv env;
        makeTheme(env);
        ThemeState s = sample();
        s.gtkTheme = "T";
        QVERIFY(GtkExport::linkGtk4(s, env.path("usr/share/themes/T")).isOk());
        QVERIFY(QFileInfo(env.path(".config/gtk-4.0/gtk.css")).isSymLink());
        QVERIFY(QFileInfo(env.path(".config/gtk-4.0/gtk-dark.css")).isSymLink());
        QVERIFY(QFileInfo(env.path(".config/gtk-4.0/assets")).isSymLink());
    }

    void neverReplacesARegularGtkCss()
    {
        TestEnv env;
        makeTheme(env);
        const QString noctalia = "@import 'noctalia.css';\n";
        TestEnv::write(env.path(".config/gtk-3.0/gtk.css"), noctalia);
        TestEnv::write(env.path(".config/gtk-4.0/gtk.css"), noctalia);
        TestEnv::write(env.path(".config/gtk-4.0/noctalia.css"), "colors");
        TestEnv::write(env.path(".config/gtk-4.0/settings.ini"), "[Settings]\nkeep=me\n");

        ThemeState s = sample();
        s.gtkTheme = "T";
        const StepResult r = GtkExport::linkGtk4(s, env.path("usr/share/themes/T"));

        QCOMPARE(r.status, StepResult::Warning);   // told the user it left something alone
        QVERIFY(!QFileInfo(env.path(".config/gtk-4.0/gtk.css")).isSymLink());
        QCOMPARE(TestEnv::read(env.path(".config/gtk-4.0/gtk.css")), noctalia);
        QCOMPARE(TestEnv::read(env.path(".config/gtk-3.0/gtk.css")), noctalia);
        QCOMPARE(TestEnv::read(env.path(".config/gtk-4.0/noctalia.css")), QString("colors"));
        // clearGtk4Symlinks() in nwg-look deleted settings.ini too; it must survive here.
        QCOMPARE(TestEnv::read(env.path(".config/gtk-4.0/settings.ini")), QString("[Settings]\nkeep=me\n"));
        // the parts that were free are still linked
        QVERIFY(QFileInfo(env.path(".config/gtk-4.0/gtk-dark.css")).isSymLink());
    }

    void clearOnlyRemovesThemeSymlinks()
    {
        TestEnv env;
        makeTheme(env);
        // ours: points into a themes folder
        QDir().mkpath(env.path(".config/gtk-4.0"));
        QVERIFY(QFile::link(env.path("usr/share/themes/T/gtk-4.0/gtk-dark.css"), env.path(".config/gtk-4.0/gtk-dark.css")));
        // not ours: a dotfile-manager link
        TestEnv::write(env.path("dotfiles/gtk.css"), "mine");
        QVERIFY(QFile::link(env.path("dotfiles/gtk.css"), env.path(".config/gtk-4.0/gtk.css")));
        // regular directory
        QDir().mkpath(env.path(".config/assets"));

        QVERIFY(GtkExport::clearGtk4Symlinks().isOk());

        QVERIFY(!QFileInfo(env.path(".config/gtk-4.0/gtk-dark.css")).isSymLink());
        QVERIFY(!QFileInfo::exists(env.path(".config/gtk-4.0/gtk-dark.css")));
        QVERIFY(QFileInfo(env.path(".config/gtk-4.0/gtk.css")).isSymLink());
        QVERIFY(QFileInfo(env.path(".config/assets")).isDir());
    }

    void reportsAThemeWithoutGtk4()
    {
        TestEnv env;
        QDir().mkpath(env.path("usr/share/themes/Old/gtk-3.0"));
        ThemeState s = sample();
        s.gtkTheme = "Old";
        QCOMPARE(GtkExport::linkGtk4(s, env.path("usr/share/themes/Old")).status, StepResult::Warning);
    }
};

QTEST_MAIN(TstGtkExport)
#include "tst_gtkexport.moc"
