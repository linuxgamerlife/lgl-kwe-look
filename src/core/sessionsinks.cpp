#include "sessionsinks.h"

#include "flatpak.h"
#include "gsettingsclient.h"
#include "gtkexport.h"
#include "kwepaths.h"
#include "session.h"

#include <QDBusConnection>
#include <QDBusMessage>

namespace Sinks {

namespace {

// Raw KConfig-style write: theme names are plain, no QSettings quoting.
StepResult writeRaw(const QString &path, const QList<IniFile::Update> &updates, const QString &what)
{
    QString err;
    if (!IniFile::applyUpdates(path, updates, &err))
        return StepResult::fail(err);
    return StepResult::ok(what);
}

void gs(ApplyPlan &plan, const QString &schema, const QString &key, const QString &value)
{
    plan.add(ApplyOrder::GSettings, QStringLiteral("gsettings %1").arg(key),
             [=]() { return GSettings::setVerified(schema, key, value); });
}

}  // namespace

StepResult notifyKGlobalSettings(int type, int arg)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return StepResult::warn(QStringLiteral("no session bus, running Qt/KDE apps were not notified"));

    QDBusMessage msg = QDBusMessage::createSignal(QStringLiteral("/KGlobalSettings"),
                                                  QStringLiteral("org.kde.KGlobalSettings"),
                                                  QStringLiteral("notifyChange"));
    msg << type << arg;
    if (!bus.send(msg))
        return StepResult::warn(QStringLiteral("KGlobalSettings notification could not be sent"));
    return StepResult::ok();
}

void addQtCtSteps(ApplyPlan &plan, QtTarget target, bool linked,
                  const QList<IniFile::Update> &updates, const QString &what)
{
    if (updates.isEmpty())
        return;
    const QList<QtTarget> targets = linked ? QList<QtTarget>{QtTarget::Qt5, QtTarget::Qt6}
                                           : QList<QtTarget>{target};
    for (QtTarget t : targets) {
        plan.add(ApplyOrder::QtCtConfig, QStringLiteral("%1.conf %2").arg(QtCt::toolName(t), what),
                 [=]() { return QtCt::applyUpdates(t, updates, what); });
    }
}

void addSessionSteps(ApplyPlan &plan, const ThemeState &state, const ThemeState &baseline, bool force)
{
    const bool kwe = KwePaths::kweConfigPresent();

    if (force || state.iconTheme != baseline.iconTheme) {
        const QString icon = state.iconTheme;

        if (kwe) {
            plan.add(ApplyOrder::AppearanceKwe, QStringLiteral("appearance.kwe [Appearance] IconTheme"),
                     [=]() {
                         return writeRaw(KwePaths::appearanceKwe(),
                                         {IniFile::Update::set(QStringLiteral("Appearance"),
                                                               QStringLiteral("IconTheme"), icon)},
                                         icon);
                     });
        }
        if (Session::writesKdeGlobals()) {
            plan.add(ApplyOrder::KdeGlobals, QStringLiteral("kdeglobals [Icons] Theme"), [=]() {
                return writeRaw(KwePaths::kdeglobals(),
                                {IniFile::Update::set(QStringLiteral("Icons"), QStringLiteral("Theme"), icon)},
                                icon);
            });
        }
        // Both tool configs regardless of the link toggle: the session mirrors both. A full
        // desktop with its own Qt platform theme only gets them when qt5ct/qt6ct are in use.
        for (QtTarget t : {QtTarget::Qt5, QtTarget::Qt6}) {
            if (!Session::mirrorsQtCt(t))
                continue;
            plan.add(ApplyOrder::QtCtConfig,
                     QStringLiteral("%1.conf icon_theme").arg(QtCt::toolName(t)), [=]() {
                         return QtCt::applyUpdates(
                             t, {QtCt::setString(QStringLiteral("Appearance"), QStringLiteral("icon_theme"), icon)},
                             icon);
                     });
        }
        gs(plan, QLatin1String(GSettings::kInterface), QStringLiteral("icon-theme"), icon);
        plan.add(ApplyOrder::Notify, QStringLiteral("notify Qt/KDE apps (icons)"),
                 []() { return notifyKGlobalSettings(IconChanged); });
    }

    if (force || state.cursorTheme != baseline.cursorTheme || state.cursorSize != baseline.cursorSize) {
        const QString theme = state.cursorTheme;
        const int size = state.cursorSize;

        if (kwe) {
            plan.add(ApplyOrder::AppearanceKwe,
                     QStringLiteral("appearance.kwe [Appearance] CursorTheme/CursorSize"), [=]() {
                         return writeRaw(KwePaths::appearanceKwe(),
                                         {IniFile::Update::set(QStringLiteral("Appearance"),
                                                               QStringLiteral("CursorTheme"), theme),
                                          IniFile::Update::set(QStringLiteral("Appearance"),
                                                               QStringLiteral("CursorSize"),
                                                               QString::number(size))},
                                         QStringLiteral("%1 %2").arg(theme).arg(size));
                     });
            // What the compositor actually reads.
            plan.add(ApplyOrder::CompositorKwe,
                     QStringLiteral("kineticwe.kwe [Mouse] cursorTheme/cursorSize"), [=]() {
                         return writeRaw(KwePaths::kineticweKwe(),
                                         {IniFile::Update::set(QStringLiteral("Mouse"),
                                                               QStringLiteral("cursorTheme"), theme),
                                          IniFile::Update::set(QStringLiteral("Mouse"),
                                                               QStringLiteral("cursorSize"),
                                                               QString::number(size))},
                                         QStringLiteral("%1 %2").arg(theme).arg(size));
                     });
        }
        if (Session::writesKdeGlobals()) {
            plan.add(ApplyOrder::KdeGlobals, QStringLiteral("kdeglobals [KDE] cursorTheme/cursorSize"),
                     [=]() {
                         return writeRaw(KwePaths::kdeglobals(),
                                         {IniFile::Update::set(QStringLiteral("KDE"),
                                                               QStringLiteral("cursorTheme"), theme),
                                          IniFile::Update::set(QStringLiteral("KDE"),
                                                               QStringLiteral("cursorSize"),
                                                               QString::number(size))},
                                         QStringLiteral("%1 %2").arg(theme).arg(size));
                     });
        }
        if (Session::writesKcminputrc()) {
            // Plasma keeps the pointer theme here, not in kdeglobals.
            plan.add(ApplyOrder::KdeGlobals, QStringLiteral("kcminputrc [Mouse] cursorTheme/cursorSize"),
                     [=]() {
                         return writeRaw(KwePaths::kcminputrc(),
                                         {IniFile::Update::set(QStringLiteral("Mouse"),
                                                               QStringLiteral("cursorTheme"), theme),
                                          IniFile::Update::set(QStringLiteral("Mouse"),
                                                               QStringLiteral("cursorSize"),
                                                               QString::number(size))},
                                         QStringLiteral("%1 %2").arg(theme).arg(size));
                     });
        }
        gs(plan, QLatin1String(GSettings::kInterface), QStringLiteral("cursor-theme"), theme);
        gs(plan, QLatin1String(GSettings::kInterface), QStringLiteral("cursor-size"), QString::number(size));
        plan.add(ApplyOrder::Notify, QStringLiteral("notify compositor and apps (cursor)"),
                 []() { return notifyKGlobalSettings(CursorChanged); });
    }
}

void addGSettingsSteps(ApplyPlan &plan, const ThemeState &s, const ThemeState &b, bool force,
                       bool includeSession)
{
    const QString iface = QLatin1String(GSettings::kInterface);
    const QString sound = QLatin1String(GSettings::kSound);
    const auto boolText = [](bool v) { return v ? QStringLiteral("true") : QStringLiteral("false"); };

    if (force || s.gtkTheme != b.gtkTheme)               gs(plan, iface, QStringLiteral("gtk-theme"), s.gtkTheme);
    if (force || s.fontName != b.fontName)               gs(plan, iface, QStringLiteral("font-name"), s.fontName);
    if (force || s.toolbarStyle != b.toolbarStyle)       gs(plan, iface, QStringLiteral("toolbar-style"), s.toolbarStyle);
    if (force || s.toolbarIconsSize != b.toolbarIconsSize)
        gs(plan, iface, QStringLiteral("toolbar-icons-size"), s.toolbarIconsSize);
    if (force || s.fontHinting != b.fontHinting)         gs(plan, iface, QStringLiteral("font-hinting"), s.fontHinting);
    if (force || s.fontAntialiasing != b.fontAntialiasing)
        gs(plan, iface, QStringLiteral("font-antialiasing"), s.fontAntialiasing);
    if (force || s.fontRgbaOrder != b.fontRgbaOrder)     gs(plan, iface, QStringLiteral("font-rgba-order"), s.fontRgbaOrder);
    if (force || qAbs(s.textScalingFactor - b.textScalingFactor) > 1e-6)
        gs(plan, iface, QStringLiteral("text-scaling-factor"), QString::number(s.textScalingFactor, 'f', 6));
    if (force || s.colorScheme != b.colorScheme)         gs(plan, iface, QStringLiteral("color-scheme"), s.colorScheme);
    if (force || s.eventSounds != b.eventSounds)         gs(plan, sound, QStringLiteral("event-sounds"), boolText(s.eventSounds));
    if (force || s.inputFeedbackSounds != b.inputFeedbackSounds)
        gs(plan, sound, QStringLiteral("input-feedback-sounds"), boolText(s.inputFeedbackSounds));

    if (includeSession) {
        gs(plan, iface, QStringLiteral("icon-theme"), s.iconTheme);
        gs(plan, iface, QStringLiteral("cursor-theme"), s.cursorTheme);
        gs(plan, iface, QStringLiteral("cursor-size"), QString::number(s.cursorSize));
    }
}

void addGtkSteps(ApplyPlan &plan, const GtkContext &ctx)
{
    const ThemeState s = ctx.state;
    const Preferences p = ctx.prefs;
    const bool prefsChanged = ctx.prefs != ctx.prefsBaseline;
    const bool anyChange = ctx.force || ctx.exportOnly || prefsChanged
                        || s.gtkDiffers(ctx.baseline) || s.sessionDiffers(ctx.baseline);

    if (!ctx.exportOnly)
        addGSettingsSteps(plan, s, ctx.baseline, ctx.force, false);

    if (!anyChange)
        return;

    const QStringList preserved = ctx.preservedIniLines;
    if (p.exportSettingsIni)
        plan.add(ApplyOrder::GtkExport, QStringLiteral("export gtk-3.0 / gtk-4.0 settings.ini"),
                 [=]() { return GtkExport::writeSettingsIni(s, preserved); });
    if (p.exportGtkrc2)
        plan.add(ApplyOrder::GtkExport, QStringLiteral("export gtkrc-2.0"),
                 [=]() { return GtkExport::writeGtkrc2(s); });
    if (p.exportXsettingsd)
        plan.add(ApplyOrder::GtkExport, QStringLiteral("export xsettingsd.conf"),
                 [=]() { return GtkExport::writeXsettingsd(s); });
    if (p.exportIndexTheme)
        plan.add(ApplyOrder::GtkExport, QStringLiteral("export ~/.icons/default/index.theme"),
                 [=]() { return GtkExport::writeIndexTheme(s); });

    const QString themePath = ctx.gtkThemePath;
    if (p.exportGtk4Symlinks)
        plan.add(ApplyOrder::GtkExport, QStringLiteral("link GTK4 theme files"),
                 [=]() { return GtkExport::linkGtk4(s, themePath); });
    else
        plan.add(ApplyOrder::GtkExport, QStringLiteral("remove GTK4 theme symlinks we created"),
                 []() { return GtkExport::clearGtk4Symlinks(); });

    plan.add(ApplyOrder::GtkExport, QStringLiteral("save gsettings backup"), [=]() {
        QString err;
        if (!ThemeStateIO::saveBackup(s, &err))
            return StepResult::warn(err);
        return StepResult::ok(ThemeStateIO::backupPath());
    });

    // Flatpak: only when something it depends on changed, it is slow and often absent.
    const bool flatpakRelevant = ctx.force || prefsChanged || s.gtkTheme != ctx.baseline.gtkTheme
                              || s.iconTheme != ctx.baseline.iconTheme;
    if (!ctx.exportOnly && flatpakRelevant && Flatpak::available()) {
        const auto gtkName = QStringLiteral("GTK_THEME");
        const auto iconName = QStringLiteral("ICON_THEME");
        plan.add(ApplyOrder::Flatpak, QStringLiteral("flatpak GTK_THEME override"), [=]() {
            return p.flatpakGtkThemeOverride ? Flatpak::overrideEnv(gtkName, s.gtkTheme)
                                             : Flatpak::unsetEnv(gtkName);
        });
        plan.add(ApplyOrder::Flatpak, QStringLiteral("flatpak ICON_THEME override"), [=]() {
            return p.flatpakIconThemeOverride ? Flatpak::overrideEnv(iconName, s.iconTheme)
                                              : Flatpak::unsetEnv(iconName);
        });
        if (p.flatpakInstallCurrentTheme)
            plan.add(ApplyOrder::Flatpak, QStringLiteral("install GTK theme for Flatpak"),
                     [=]() { return Flatpak::installUserTheme(s.gtkTheme); });
    }
}

}  // namespace Sinks
