#include "gtkexportpage.h"

#include "appmodel.h"
#include "core/flatpak.h"
#include "core/gtkexport.h"
#include "core/kwepaths.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QLabel>

GtkExportPage::GtkExportPage(AppModel *model, QWidget *parent)
    : PageBase(parent), m_model(model), m_bind(model)
{
    auto *root = initPage(tr("Export and Flatpak"),
                          tr("GTK reads its settings from files as well as from gsettings. Choose which files "
                             "are written when you apply."));

    QVBoxLayout *el = nullptr;
    QFrame *exports = makePanel(this, &el);
    el->addWidget(makeSectionTitle(exports, tr("Export files")));

    const auto addPref = [&](const QString &label, const QString &desc, bool Preferences::*field) {
        auto *cb = new QCheckBox(label, exports);
        el->addWidget(cb);
        el->addWidget(makeDescLabel(exports, desc));
        m_bind.checkPref(cb, field);
        return cb;
    };
    addPref(tr("gtk-3.0 and gtk-4.0 settings.ini"),
            tr("Read by GTK3 and GTK4 apps that do not use gsettings. Lines this app does not manage are kept."),
            &Preferences::exportSettingsIni);
    addPref(tr("~/.gtkrc-2.0"), tr("For GTK2 apps. Put your own additions in ~/.gtkrc-2.0.mine."),
            &Preferences::exportGtkrc2);
    addPref(tr("~/.icons/default/index.theme"),
            tr("Makes the cursor theme apply to X11 and XWayland apps that only read the default theme."),
            &Preferences::exportIndexTheme);
    addPref(tr("xsettingsd.conf"), tr("For X11 apps when xsettingsd is running."), &Preferences::exportXsettingsd);
    auto *gtk4 = addPref(tr("Link GTK4 theme files into ~/.config/gtk-4.0"),
                         tr("Symlinks the theme's gtk.css, gtk-dark.css and assets. Regular files are never replaced."),
                         &Preferences::exportGtk4Symlinks);
    Q_UNUSED(gtk4)

    m_gtk4Note = new QLabel(exports);
    m_gtk4Note->setWordWrap(true);
    m_gtk4Note->setTextFormat(Qt::RichText);
    el->addWidget(m_gtk4Note);

    auto *exportNow = makeToolbarBtn(tr("Export files now"), exports);
    exportNow->setToolTip(tr("Write the files above from the current settings without changing gsettings."));
    connect(exportNow, &QPushButton::clicked, this, &GtkExportPage::exportNowRequested);
    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(exportNow);
    btnRow->addStretch();
    el->addLayout(btnRow);
    root->addWidget(exports);

    QVBoxLayout *fl = nullptr;
    QFrame *flatpak = makePanel(this, &fl);
    fl->addWidget(makeSectionTitle(flatpak, tr("Flatpak")));
    makeStatusRow(flatpak, fl, tr("flatpak command"), Flatpak::available(), tr("[Installed]"), tr("[Not Installed]"));

    const auto addFlatpak = [&](const QString &label, const QString &desc, bool Preferences::*field) {
        auto *cb = new QCheckBox(label, flatpak);
        cb->setEnabled(Flatpak::available());
        fl->addWidget(cb);
        fl->addWidget(makeDescLabel(flatpak, desc));
        m_bind.checkPref(cb, field);
    };
    addFlatpak(tr("Override GTK_THEME for Flatpak apps"),
               tr("flatpak override --user --env=GTK_THEME=<theme>. Turning it off removes the override."),
               &Preferences::flatpakGtkThemeOverride);
    addFlatpak(tr("Override ICON_THEME for Flatpak apps"),
               tr("Not a standard variable, kept from nwg-look. Leave off unless an app needs it."),
               &Preferences::flatpakIconThemeOverride);
    addFlatpak(tr("Install the current GTK theme for Flatpak apps"),
               tr("Copies the theme into ~/.themes and gives Flatpak read access to it."),
               &Preferences::flatpakInstallCurrentTheme);
    root->addWidget(flatpak);
    root->addStretch();

    connect(m_model, &AppModel::changed, this, &GtkExportPage::updateGtk4Note);
    connect(m_model, &AppModel::reloaded, this, &GtkExportPage::load);
    load();
}

void GtkExportPage::load()
{
    m_bind.refresh();
    updateGtk4Note();
}

void GtkExportPage::updateGtk4Note()
{
    QStringList regular;
    for (const char *rel : {"gtk-3.0/gtk.css", "gtk-4.0/gtk.css"}) {
        const QFileInfo fi(KwePaths::realConfigHome() + QLatin1Char('/') + QLatin1String(rel));
        if (fi.exists() && !fi.isSymLink())
            regular << QLatin1String(rel);
    }

    QString text;
    if (!regular.isEmpty()) {
        text = tr("<span style='color:#cc7700;'><b>Note:</b></span> %1 exist as regular files (Noctalia keeps its "
                  "colours there). They are never modified, and the theme's own gtk.css will not be linked over them.")
                   .arg(regular.join(QStringLiteral(", ")));
    } else if (m_model->prefs.exportGtk4Symlinks && KwePaths::isKineticWeSession()) {
        text = tr("<span style='color:#cc7700;'><b>Note:</b></span> If Noctalia later writes gtk.css it would write "
                  "through these symlinks into the theme folder. Keep this off unless you are not using Noctalia's "
                  "GTK colours.");
    }
    m_gtk4Note->setText(text);
    m_gtk4Note->setVisible(!text.isEmpty());
}
