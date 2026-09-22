#include "appmodel.h"

AppModel::AppModel(QObject *parent) : QObject(parent) {}

void AppModel::load()
{
    baseline = ThemeStateIO::load(&preservedIniLines);
    state = baseline;
    prefs = Prefs::load();
    prefsBaseline = prefs;
    forceReapply = false;
    rescanThemes();
    emit reloaded();
    emit changed();
}

void AppModel::rescanThemes()
{
    gtkThemes = ThemeScan::gtkThemes();
    iconThemes = ThemeScan::iconThemes();
    cursorThemes = ThemeScan::cursorThemes();
}

void AppModel::commit()
{
    baseline = state;
    prefsBaseline = prefs;
    forceReapply = false;
    QString err;
    Prefs::save(prefs, &err);
    // The file we just wrote is now the source of the preserved lines.
    ThemeState scratch;
    ThemeStateIO::readIniExtras(&scratch, &preservedIniLines);
    emit changed();
}

QString AppModel::gtkThemePath(const QString &name) const
{
    for (const ThemeScan::GtkTheme &t : gtkThemes) {
        if (t.name == name)
            return t.path;
    }
    return QString();
}
