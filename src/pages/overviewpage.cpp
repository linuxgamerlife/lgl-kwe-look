#include "overviewpage.h"

#include "appmodel.h"
#include "core/flatpak.h"
#include "core/gsettingsclient.h"
#include "core/kwepaths.h"
#include "core/qtct.h"
#include "core/session.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLabel>
#include <QLocale>
#include <QUrl>

namespace {

QString helperPath()
{
    const QString beside = QCoreApplication::applicationDirPath() + QStringLiteral("/lgl-kwe-look-gtk-preview");
    if (QFileInfo::exists(beside))
        return beside;
    return QStringLiteral(LGLKWE_LIBEXECDIR "/lgl-kwe-look-gtk-preview");
}

bool platformThemeInstalled(QtTarget t)
{
    const QString root = QtCt::pluginRoot(t);
    return !root.isEmpty()
        && QFileInfo::exists(root + QStringLiteral("/platformthemes/lib%1.so").arg(QtCt::toolName(t)));
}

}  // namespace

OverviewPage::OverviewPage(AppModel *model, QWidget *parent) : PageBase(parent), m_model(model)
{
    auto *root = initPage(tr("Welcome to LGL KWE Look"),
                          tr("One place for the GTK, Qt5 and Qt6 appearance of your session. Built for KineticWE, "
                             "and it detects other desktops such as KDE and GNOME. It replaces nwg-look, qt5ct "
                             "and qt6ct."));

    // ---- session ----
    QVBoxLayout *sl = nullptr;
    QFrame *session = makePanel(this, &sl);
    sl->addWidget(new QLabel(tr("<h3>Session</h3>")));

    const Session::Info info = Session::detect();
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP", tr("unset"));
    if (info.type == Session::Type::Other) {
        makeStatusRow(this, sl, tr("No known desktop session (XDG_CURRENT_DESKTOP=%1)").arg(desktop), false,
                      tr("[Detected]"), tr("[Not Detected]"));
    } else {
        const QString what = info.display.isEmpty() ? tr("%1 session detected").arg(info.name)
                                                    : tr("%1 session detected (%2)").arg(info.name, info.display);
        makeStatusRow(this, sl, tr("%1, XDG_CURRENT_DESKTOP=%2").arg(what, desktop), true, tr("[Detected]"),
                      tr("[Not Detected]"));
    }
    if (info.type == Session::Type::KineticWE || KwePaths::kweConfigPresent())
        sl->addWidget(new QLabel(tr("<b>KineticWE settings:</b> %1").arg(KwePaths::kweConfigHome().toHtmlEscaped())));
    sl->addWidget(new QLabel(tr("<b>QT_QPA_PLATFORMTHEME:</b> %1")
                                 .arg(qEnvironmentVariable("QT_QPA_PLATFORMTHEME", tr("unset")).toHtmlEscaped())));

    QString text;
    switch (info.type) {
    case Session::Type::KineticWE:
        text = tr("In a KineticWE session <i>appearance.kwe</i> is the source of truth for the icon and cursor "
                  "theme. Apply writes it together with the compositor config, kdeglobals, qt5ct/qt6ct and "
                  "gsettings, so the choice survives the next login.");
        break;
    case Session::Type::Kde:
        text = tr("In a KDE session Apply writes the icon and cursor theme to <i>kdeglobals</i> and "
                  "<i>kcminputrc</i> and notifies Plasma, next to the GTK settings. Plasma uses its own Qt "
                  "platform theme, so the qt5ct and qt6ct pages only take effect when QT_QPA_PLATFORMTHEME is "
                  "set to qt5ct or qt6ct.");
        break;
    case Session::Type::Other:
        text = tr("Apply writes gsettings, the GTK files and qt5ct/qt6ct. Desktop specific files such as "
                  "kdeglobals are only written when they already exist.");
        break;
    default:
        text = tr("Apply writes gsettings and the GTK files. %1 has its own Qt platform theme, so qt5ct and qt6ct "
                  "are only edited when they are already set up or selected with QT_QPA_PLATFORMTHEME. "
                  "%1's own settings tool is not written to directly.")
                   .arg(info.name);
        break;
    }
    auto *note = new QLabel(text);
    note->setWordWrap(true);
    sl->addWidget(note);
    root->addWidget(session);

    // ---- output ----
    QVBoxLayout *ol0 = nullptr;
    QFrame *outputs = makePanel(this, &ol0);
    ol0->addWidget(new QLabel(tr("<h3>Output</h3>")));
    auto *outputNote = new QLabel(tr("The files and settings that Apply writes in this session. Click a path to "
                                     "select it, or open its folder."));
    outputNote->setWordWrap(true);
    ol0->addWidget(outputNote);
    m_outputsLayout = new QVBoxLayout;
    ol0->addLayout(m_outputsLayout);
    root->addWidget(outputs);
    buildOutputRows();
    // The list follows the export options and the link toggle.
    connect(m_model, &AppModel::changed, this, &OverviewPage::buildOutputRows);
    connect(m_model, &AppModel::reloaded, this, &OverviewPage::buildOutputRows);

    // ---- components ----
    QVBoxLayout *cl = nullptr;
    QFrame *components = makePanel(this, &cl);
    cl->addWidget(new QLabel(tr("<h3>Components</h3>")));
    m_componentsLayout = cl;
    root->addWidget(components);
    buildComponentRows();

    // ---- options ----
    QVBoxLayout *ol = nullptr;
    QFrame *options = makePanel(this, &ol);
    ol->addWidget(new QLabel(tr("<h3>Options</h3>")));
    m_link = new QCheckBox(tr("Link Qt5 and Qt6 settings (write qt5ct.conf and qt6ct.conf together)"));
    ol->addWidget(withHelp(m_link, tr("When on, one set of Qt pages edits both qt5ct.conf and qt6ct.conf so Qt5 and "
                                      "Qt6 apps look the same. Turn it off to give each its own settings.")));
    ol->addWidget(makeDescLabel(this, tr("Matches how the session treats both files. Turn it off to edit the Qt5 "
                                         "and Qt6 pages independently.")));
    connect(m_link, &QCheckBox::toggled, this, [this](bool on) {
        m_model->prefs.linkQt = on;
        m_model->touch();
        emit m_model->linkChanged(on);
    });

    auto *buttons = new QHBoxLayout;
    auto *reapply = makeToolbarBtn(tr("Re-apply all settings"));
    reapply->setToolTip(tr("Write every sink again, not only what changed. Use it after another tool "
                           "overwrote a config file."));
    connect(reapply, &QPushButton::clicked, this, &OverviewPage::reapplyRequested);
    auto *reload = makeToolbarBtn(tr("Reload from disk"));
    connect(reload, &QPushButton::clicked, this, &OverviewPage::reloadRequested);
    buttons->addWidget(reapply);
    buttons->addWidget(reload);
    buttons->addStretch();
    ol->addLayout(buttons);
    root->addWidget(options);

    root->addStretch();
    load();
}

void OverviewPage::load()
{
    const QSignalBlocker b(m_link);
    m_link->setChecked(m_model->prefs.linkQt);
}

void OverviewPage::buildOutputRows()
{
    while (QLayoutItem *item = m_outputsLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }

    const QStringList groupTitles{tr("Desktop"), tr("GTK"), tr("Qt")};
    int lastGroup = -1;
    for (const Session::Output &o : Session::outputs(m_model->prefs)) {
        if (int(o.group) != lastGroup) {
            lastGroup = int(o.group);
            m_outputsLayout->addWidget(makeSectionTitle(this, groupTitles.at(lastGroup)));
        }

        auto *row = new QWidget;
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(8);

        auto *text = new QLabel(QStringLiteral("<b>%1</b> &nbsp; %2").arg(o.what.toHtmlEscaped(), o.path.toHtmlEscaped()),
                                row);
        text->setTextFormat(Qt::RichText);
        text->setTextInteractionFlags(Qt::TextSelectableByMouse);
        text->setWordWrap(true);
        rl->addWidget(text, 1);

        if (o.isFile) {
            auto *badge = new QLabel(o.exists ? tr("[Exists]") : tr("[Created on Apply]"), row);
            badge->setStyleSheet(badgeStyle(o.exists));
            rl->addWidget(badge);

            const QFileInfo fi(o.path);
            const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
            auto *open = makeToolbarBtn(tr("Open folder"), row);
            open->setEnabled(QFileInfo(dir).isDir());
            connect(open, &QPushButton::clicked, this,
                    [dir]() { QDesktopServices::openUrl(QUrl::fromLocalFile(dir)); });
            rl->addWidget(open);
        }
        m_outputsLayout->addWidget(row);
    }
}

void OverviewPage::buildComponentRows()
{
    // Probes touch the file system and run `flatpak`/`gsettings` lookups, so they run off the GUI thread.
    auto *layout = m_componentsLayout;
    auto *status = new QLabel(tr("Checking…"));
    layout->addWidget(status);

    runChecksAsync(
        this,
        {{QStringLiteral("gsettings"), []() { return GSettings::available(); }},
         {QStringLiteral("qt5ct"), []() { return platformThemeInstalled(QtTarget::Qt5); }},
         {QStringLiteral("qt6ct"), []() { return platformThemeInstalled(QtTarget::Qt6); }},
         {QStringLiteral("preview"), []() { return QFileInfo::exists(helperPath()); }},
         {QStringLiteral("flatpak"), []() { return Flatpak::available(); }}},
        [this, layout, status](QMap<QString, bool> r) {
            delete status;
            makeStatusRow(this, layout, tr("gsettings (GTK settings backend)"), r.value(QStringLiteral("gsettings")),
                          tr("[Installed]"), tr("[Not Installed]"));
            makeStatusRow(this, layout, tr("qt5ct platform theme (Fedora package qt5ct)"),
                          r.value(QStringLiteral("qt5ct")), tr("[Installed]"), tr("[Not Installed]"));
            makeStatusRow(this, layout, tr("qt6ct platform theme (Fedora package qt6ct)"),
                          r.value(QStringLiteral("qt6ct")), tr("[Installed]"), tr("[Not Installed]"));
            makeStatusRow(this, layout, tr("GTK preview helper"), r.value(QStringLiteral("preview")),
                          tr("[Installed]"), tr("[Not Installed]"));
            makeStatusRow(this, layout, tr("Flatpak (optional overrides)"), r.value(QStringLiteral("flatpak")),
                          tr("[Installed]"), tr("[Not Installed]"));
            auto *hint = new QLabel(tr("The qt5ct and qt6ct packages provide the runtime plugins that apply the "
                                       "settings inside Qt apps. This app only edits their configuration."));
            hint->setWordWrap(true);
            layout->addWidget(hint);
        });
}
