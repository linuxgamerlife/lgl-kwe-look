#include "gtkotherpage.h"

#include "appmodel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>

GtkOtherPage::GtkOtherPage(AppModel *model, QWidget *parent)
    : PageBase(parent), m_model(model), m_bind(model)
{
    auto *root = initPage(tr("Other GTK Settings"), tr("Toolbars, sounds and legacy settings.ini values."));

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(this, &pl);
    pl->addWidget(makeSectionTitle(panel, tr("gsettings")));
    auto *form = new QFormLayout;

    auto *toolbar = new QComboBox(panel);
    toolbar->addItem(tr("Icons only"), QStringLiteral("icons"));
    toolbar->addItem(tr("Text only"), QStringLiteral("text"));
    toolbar->addItem(tr("Text below icons"), QStringLiteral("both"));
    toolbar->addItem(tr("Text beside icons"), QStringLiteral("both-horiz"));
    form->addRow(tr("Toolbar style"),
                 withHelp(toolbar, tr("Whether toolbar buttons in GTK apps show icons, text, or both. Apps may "
                                      "override it for their own toolbars.")));

    auto *iconSize = new QComboBox(panel);
    iconSize->addItem(tr("Small"), QStringLiteral("small"));
    iconSize->addItem(tr("Large"), QStringLiteral("large"));
    form->addRow(tr("Toolbar icon size"),
                 withHelp(iconSize, tr("The size of icons on GTK toolbars: small or large.")));
    pl->addLayout(form);

    auto *events = new QCheckBox(tr("Enable event sounds"), panel);
    auto *feedback = new QCheckBox(tr("Enable input feedback sounds"), panel);
    pl->addWidget(withHelp(events, tr("Lets GTK apps play a sound for events such as an alert or an error. It "
                                      "only has an effect where a sound theme is installed.")));
    pl->addWidget(withHelp(feedback, tr("Lets GTK apps play a short sound as you click or type. Off by default on "
                                        "most desktops.")));
    root->addWidget(panel);

    QVBoxLayout *ll = nullptr;
    QFrame *legacy = makePanel(this, &ll);
    ll->addWidget(makeSectionTitle(legacy, tr("settings.ini and gtkrc-2.0 only")));
    ll->addWidget(makeDescLabel(legacy, tr("GTK ignores or has deprecated these, they are kept for older "
                                           "toolkits and for compatibility with lxappearance-style files.")));
    auto *lform = new QFormLayout;
    auto *iniStyle = new QComboBox(legacy);
    iniStyle->addItem(tr("Icons only"), QStringLiteral("GTK_TOOLBAR_ICONS"));
    iniStyle->addItem(tr("Text only"), QStringLiteral("GTK_TOOLBAR_TEXT"));
    iniStyle->addItem(tr("Text below icons"), QStringLiteral("GTK_TOOLBAR_BOTH"));
    iniStyle->addItem(tr("Text beside icons"), QStringLiteral("GTK_TOOLBAR_BOTH_HORIZ"));
    lform->addRow(tr("Toolbar style"),
                  withHelp(iniStyle, tr("The toolbar style written to settings.ini and gtkrc-2.0, for GTK2 apps and "
                                        "older GTK3 apps.")));
    auto *iniSize = new QComboBox(legacy);
    for (const char *s : {"GTK_ICON_SIZE_MENU", "GTK_ICON_SIZE_SMALL_TOOLBAR", "GTK_ICON_SIZE_LARGE_TOOLBAR",
                          "GTK_ICON_SIZE_BUTTON", "GTK_ICON_SIZE_DND", "GTK_ICON_SIZE_DIALOG"})
        iniSize->addItem(QLatin1String(s), QLatin1String(s));
    lform->addRow(tr("Toolbar icon size"),
                  withHelp(iniSize, tr("The toolbar icon size written to settings.ini and gtkrc-2.0. MENU is the "
                                       "smallest, DIALOG the largest.")));
    ll->addLayout(lform);
    auto *buttonImages = new QCheckBox(tr("Show images on buttons (deprecated)"), legacy);
    auto *menuImages = new QCheckBox(tr("Show images in menus (deprecated)"), legacy);
    ll->addWidget(withHelp(buttonImages, tr("Whether older GTK apps draw an icon on buttons that have both a "
                                            "label and an icon. Modern GTK ignores it.")));
    ll->addWidget(withHelp(menuImages, tr("Whether older GTK apps draw icons next to menu entries. Modern GTK "
                                          "ignores it.")));
    root->addWidget(legacy);
    root->addStretch();

    m_bind.combo(toolbar, &ThemeState::toolbarStyle);
    m_bind.combo(iconSize, &ThemeState::toolbarIconsSize);
    m_bind.check(events, &ThemeState::eventSounds);
    m_bind.check(feedback, &ThemeState::inputFeedbackSounds);
    m_bind.combo(iniStyle, &ThemeState::iniToolbarStyle);
    m_bind.combo(iniSize, &ThemeState::iniToolbarIconSize);
    m_bind.check(buttonImages, &ThemeState::buttonImages);
    m_bind.check(menuImages, &ThemeState::menuImages);

    connect(m_model, &AppModel::reloaded, this, &GtkOtherPage::load);
    load();
}

void GtkOtherPage::load()
{
    m_bind.refresh();
}
