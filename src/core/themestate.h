#pragma once
#include <QString>
#include <QStringList>

// Everything the app can apply, in one value type. The MainWindow keeps a
// working copy and a baseline; Apply writes the difference.
struct ThemeState {
    // ---- shared by GTK, Qt5 and Qt6: one choice, every sink (see sessionsinks) ----
    QString iconTheme = QStringLiteral("Adwaita");
    QString cursorTheme = QStringLiteral("Adwaita");
    int cursorSize = 24;

    // ---- GTK: org.gnome.desktop.interface / .sound ----
    QString gtkTheme = QStringLiteral("Adwaita");
    QString fontName = QStringLiteral("Sans 10");
    QString toolbarStyle = QStringLiteral("both-horiz");
    QString toolbarIconsSize = QStringLiteral("large");
    QString fontHinting = QStringLiteral("medium");
    QString fontAntialiasing = QStringLiteral("grayscale");
    QString fontRgbaOrder = QStringLiteral("rgb");
    double textScalingFactor = 1.0;
    QString colorScheme = QStringLiteral("default");
    bool eventSounds = true;
    bool inputFeedbackSounds = false;

    // ---- settings.ini / gtkrc-2.0 only (ignored or deprecated by GTK) ----
    QString iniToolbarStyle = QStringLiteral("GTK_TOOLBAR_ICONS");
    QString iniToolbarIconSize = QStringLiteral("GTK_ICON_SIZE_LARGE_TOOLBAR");
    bool buttonImages = false;
    bool menuImages = false;

    bool operator==(const ThemeState &o) const;
    bool operator!=(const ThemeState &o) const { return !(*this == o); }

    // True when the icon / cursor choice differs (the shared, multi-sink part).
    bool sessionDiffers(const ThemeState &o) const;
    // True when any GTK-only value differs.
    bool gtkDiffers(const ThemeState &o) const;
};

namespace ThemeStateIO {

// gsettings values overlaid on `base`; keys that cannot be read keep the base value.
ThemeState readGSettings(ThemeState base);
// icon/cursor from the KineticWE files (appearance.kwe first, then the compositor
// config, kdeglobals and Plasma's kcminputrc). Only the fields that are found are overwritten.
ThemeState readSessionFiles(ThemeState base);
// gtk-3.0/settings.ini extras plus the lines nwg-look would preserve verbatim.
void readIniExtras(ThemeState *s, QStringList *originalLines);
// gsettings -> session files -> settings.ini.
ThemeState load(QStringList *originalIniLines = nullptr);

// The gsettings backup, same format as nwg-look's ~/.local/share/nwg-look/gsettings.
QString backupPath();
bool saveBackup(const ThemeState &s, QString *error);
bool loadBackup(const QString &path, ThemeState *s);

}  // namespace ThemeStateIO
