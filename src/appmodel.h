#pragma once
#include <QObject>
#include <QStringList>

#include "core/preferences.h"
#include "core/themescan.h"
#include "core/themestate.h"

// Working copy of everything the top-level, GTK and shared pages edit, plus the
// baseline read from disk so Apply can write only what changed.
class AppModel : public QObject
{
    Q_OBJECT
public:
    explicit AppModel(QObject *parent = nullptr);

    // Reads gsettings, the KineticWE files, settings.ini and the preferences.
    void load();
    void rescanThemes();

    bool isDirty() const { return state != baseline || prefs != prefsBaseline; }
    // After a successful Apply.
    void commit();

    QString gtkThemePath(const QString &name) const;

    ThemeState state;
    ThemeState baseline;
    Preferences prefs;
    Preferences prefsBaseline;
    QStringList preservedIniLines;
    bool forceReapply = false;

    QList<ThemeScan::GtkTheme> gtkThemes;
    QList<ThemeScan::IconTheme> iconThemes;
    QList<ThemeScan::CursorTheme> cursorThemes;

    // Pages call this after editing state/prefs.
    void touch() { emit changed(); }

signals:
    void changed();
    void linkChanged(bool linked);
    void reloaded();
};
