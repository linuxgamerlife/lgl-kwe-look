#include "qtinterfacepage.h"

#include "appmodel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSettings>
#include <QSpinBox>

namespace {

// QPlatformTheme::KeyboardSchemes, a private-header enum with stable values.
enum KeyboardScheme { WindowsScheme = 0, MacScheme = 1, X11Scheme = 2, KdeScheme = 3, GnomeScheme = 4, CdeScheme = 5 };

}  // namespace

QCheckBox *QtInterfacePage::triState(const QString &text, QWidget *parent)
{
    // Partially checked = leave the application's own default.
    auto *cb = new QCheckBox(text, parent);
    cb->setTristate(true);
    return cb;
}

QtInterfacePage::QtInterfacePage(QtTarget target, AppModel *model, QWidget *parent)
    : QtPageBase(target, parent), m_model(model)
{
    auto *root = initPage(tr("%1 Interface").arg(QtCt::majorName(target)),
                          tr("Behaviour and effects for %1 apps.").arg(QtCt::majorName(target)));

    QVBoxLayout *gl = nullptr;
    QFrame *general = makePanel(this, &gl);
    auto *form = new QFormLayout;

    m_doubleClick = new QSpinBox(general);
    m_doubleClick->setRange(100, 2000);
    m_doubleClick->setSuffix(tr(" ms"));
    form->addRow(tr("Double click interval"),
                 withHelp(m_doubleClick, tr("The longest time between two clicks that still counts as a double "
                                            "click, in milliseconds. Raise it if double clicking is hard.")));

    m_flash = new QSpinBox(general);
    m_flash->setRange(0, 5000);
    m_flash->setSpecialValueText(tr("No blinking"));
    m_flash->setSuffix(tr(" ms"));
    form->addRow(tr("Cursor flash time"),
                 withHelp(m_flash, tr("How fast the text cursor blinks in Qt apps, in milliseconds for one full "
                                      "blink. 0 stops it blinking.")));

    m_wheel = new QSpinBox(general);
    m_wheel->setRange(1, 20);
    form->addRow(tr("Wheel scroll lines"),
                 withHelp(m_wheel, tr("How many lines a list or text area moves for one notch of the mouse "
                                      "wheel.")));

    m_buttonLayout = new QComboBox(general);
    m_buttonLayout->addItem(QStringLiteral("Windows"), int(QDialogButtonBox::WinLayout));
    m_buttonLayout->addItem(QStringLiteral("Mac OS X"), int(QDialogButtonBox::MacLayout));
    m_buttonLayout->addItem(QStringLiteral("KDE"), int(QDialogButtonBox::KdeLayout));
    m_buttonLayout->addItem(QStringLiteral("GNOME"), int(QDialogButtonBox::GnomeLayout));
    form->addRow(tr("Dialog button layout"),
                 withHelp(m_buttonLayout, tr("The order of the OK, Cancel and Help buttons in Qt dialogs. Windows "
                                             "puts OK first, KDE and GNOME follow their own habits, and Mac OS X "
                                             "puts the default button on the right.")));

    m_keyboard = new QComboBox(general);
    m_keyboard->addItem(QStringLiteral("Windows"), int(WindowsScheme));
    m_keyboard->addItem(QStringLiteral("Mac OS X"), int(MacScheme));
    m_keyboard->addItem(QStringLiteral("X11"), int(X11Scheme));
    m_keyboard->addItem(QStringLiteral("KDE"), int(KdeScheme));
    m_keyboard->addItem(QStringLiteral("GNOME"), int(GnomeScheme));
    m_keyboard->addItem(QStringLiteral("CDE"), int(CdeScheme));
    form->addRow(tr("Keyboard scheme"),
                 withHelp(m_keyboard, tr("Which platform's keyboard shortcuts Qt apps use for common actions such "
                                         "as moving between words or selecting text.")));

    m_toolButton = new QComboBox(general);
    m_toolButton->addItem(tr("Only display the icon"), int(Qt::ToolButtonIconOnly));
    m_toolButton->addItem(tr("Only display the text"), int(Qt::ToolButtonTextOnly));
    m_toolButton->addItem(tr("The text appears beside the icon"), int(Qt::ToolButtonTextBesideIcon));
    m_toolButton->addItem(tr("The text appears under the icon"), int(Qt::ToolButtonTextUnderIcon));
    m_toolButton->addItem(tr("Follow the application style"), int(Qt::ToolButtonFollowStyle));
    form->addRow(tr("Tool button style"),
                 withHelp(m_toolButton, tr("Whether toolbar buttons in Qt apps show an icon, text, or both, and "
                                           "where the text goes. Follow the application style leaves it to each "
                                           "app.")));
    gl->addLayout(form);

    m_menuIcons = new QCheckBox(tr("Show icons in menus"), general);
    m_shortcuts = new QCheckBox(tr("Show shortcuts in context menus"), general);
    m_singleClick = triState(tr("Activate items on single click"), general);
    m_dialogIcons = triState(tr("Show icons on dialog buttons"), general);
    m_underline = triState(tr("Underline keyboard shortcuts"), general);
    gl->addWidget(withHelp(m_menuIcons, tr("Shows a small icon next to menu entries that have one.")));
    gl->addWidget(withHelp(m_shortcuts, tr("Shows the keyboard shortcut next to the entries of context (right "
                                           "click) menus.")));
    gl->addWidget(withHelp(m_singleClick, tr("Opens files and items in lists with one click instead of a double "
                                             "click.")));
    gl->addWidget(withHelp(m_dialogIcons, tr("Shows an icon on the standard buttons of dialogs, such as OK and "
                                             "Cancel.")));
    gl->addWidget(withHelp(m_underline, tr("Underlines the shortcut letter in menu and button labels, for "
                                           "example the F in File.")));
    gl->addWidget(makeDescLabel(general, tr("A box in the middle state leaves the application's own default.")));
    root->addWidget(general);

    QVBoxLayout *el = nullptr;
    QFrame *effects = makePanel(this, &el);
    el->addWidget(makeSectionTitle(effects, tr("GUI effects")));
    m_effects = new QCheckBox(tr("Enable GUI effects"), effects);
    el->addWidget(withHelp(m_effects, tr("Turns the animations and fades of Qt apps on or off as a whole. Turn it "
                                         "off for a snappier feel or on a slow machine.")));
    auto *eform = new QFormLayout;
    const auto effectCombo = [&](const QString &label, const QString &help, bool hasFade) {
        auto *c = new QComboBox(effects);
        c->addItem(tr("Disabled"));
        c->addItem(tr("Animate"));
        if (hasFade)
            c->addItem(tr("Fade"));
        eform->addRow(label, withHelp(c, help));
        return c;
    };
    m_menuEffect = effectCombo(tr("Menu"), tr("How menus appear: not animated, sliding open, or fading in."), true);
    m_comboEffect = effectCombo(tr("Combo box"), tr("Whether the list of a drop-down box slides open."), false);
    m_tooltipEffect = effectCombo(tr("Tooltip"), tr("How tooltips appear: not animated, sliding in, or fading in."),
                                  true);
    m_toolboxEffect = effectCombo(tr("Tool box"), tr("Whether the pages of a tool box slide when you switch."),
                                  false);
    el->addLayout(eform);
    root->addWidget(effects);
    root->addStretch();

    const auto changed = [this]() { edited(); };
    for (QSpinBox *s : {m_doubleClick, m_flash, m_wheel})
        connect(s, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    for (QComboBox *c : {m_buttonLayout, m_keyboard, m_toolButton, m_menuEffect, m_comboEffect, m_tooltipEffect,
                         m_toolboxEffect})
        connect(c, qOverload<int>(&QComboBox::activated), this, changed);
    for (QCheckBox *c : {m_menuIcons, m_shortcuts, m_singleClick, m_dialogIcons, m_underline, m_effects})
        connect(c, &QAbstractButton::clicked, this, changed);   // user edits only; load() blocks signals

    load();
}

void QtInterfacePage::load()
{
    QSettings s(QtCt::configFile(m_target), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Interface"));

    const auto setCombo = [&](QComboBox *c, const QString &key, int def) {
        const QSignalBlocker b(c);
        const int i = c->findData(s.value(key, def).toInt());
        c->setCurrentIndex(i >= 0 ? i : c->findData(def));
    };
    const auto setTri = [&](QCheckBox *c, const QString &key) {
        const QSignalBlocker b(c);
        c->setCheckState(Qt::CheckState(s.value(key, int(Qt::PartiallyChecked)).toInt()));
    };
    const auto setSpin = [&](QSpinBox *sp, const QString &key, int def) {
        const QSignalBlocker b(sp);
        sp->setValue(s.value(key, def).toInt());
    };

    setSpin(m_doubleClick, QStringLiteral("double_click_interval"), 400);
    setSpin(m_flash, QStringLiteral("cursor_flash_time"), 1000);
    setSpin(m_wheel, QStringLiteral("wheel_scroll_lines"), 3);
    setCombo(m_buttonLayout, QStringLiteral("buttonbox_layout"), int(QDialogButtonBox::WinLayout));
    setCombo(m_keyboard, QStringLiteral("keyboard_scheme"), int(X11Scheme));
    setCombo(m_toolButton, QStringLiteral("toolbutton_style"), int(Qt::ToolButtonFollowStyle));

    {
        const QSignalBlocker b1(m_menuIcons);
        const QSignalBlocker b2(m_shortcuts);
        m_menuIcons->setChecked(s.value(QStringLiteral("menus_have_icons"), true).toBool());
        m_shortcuts->setChecked(s.value(QStringLiteral("show_shortcuts_in_context_menus"), true).toBool());
    }
    setTri(m_singleClick, QStringLiteral("activate_item_on_single_click"));
    setTri(m_dialogIcons, QStringLiteral("dialog_buttons_have_icons"));
    setTri(m_underline, QStringLiteral("underline_shortcut"));

    // gui_effects present (even empty) means "exactly these"; absent means the defaults.
    const bool hasEffects = s.childKeys().contains(QStringLiteral("gui_effects"));
    const QStringList list = s.value(QStringLiteral("gui_effects")).toStringList();
    const auto has = [&](const char *name) { return !hasEffects || list.contains(QLatin1String(name)); };
    const auto setIdx = [](QComboBox *c, int i) {
        const QSignalBlocker b(c);
        c->setCurrentIndex(i);
    };
    {
        const QSignalBlocker b(m_effects);
        m_effects->setChecked(has("General"));
    }
    setIdx(m_menuEffect, hasEffects ? (list.contains(QLatin1String("AnimateMenu")) ? 1
                                       : list.contains(QLatin1String("FadeMenu")) ? 2 : 0) : 0);
    setIdx(m_comboEffect, hasEffects && list.contains(QLatin1String("AnimateCombo")) ? 1 : 0);
    setIdx(m_tooltipEffect, hasEffects ? (list.contains(QLatin1String("AnimateTooltip")) ? 1
                                          : list.contains(QLatin1String("FadeTooltip")) ? 2 : 0) : 0);
    setIdx(m_toolboxEffect, hasEffects && list.contains(QLatin1String("AnimateToolBox")) ? 1 : 0);
    s.endGroup();
    loaded();
}

QVariantMap QtInterfacePage::snapshot() const
{
    QVariantMap m;
    m[QStringLiteral("double_click_interval")] = m_doubleClick->value();
    m[QStringLiteral("cursor_flash_time")] = m_flash->value();
    m[QStringLiteral("wheel_scroll_lines")] = m_wheel->value();
    m[QStringLiteral("buttonbox_layout")] = m_buttonLayout->currentData();
    m[QStringLiteral("keyboard_scheme")] = m_keyboard->currentData();
    m[QStringLiteral("toolbutton_style")] = m_toolButton->currentData();
    m[QStringLiteral("menus_have_icons")] = m_menuIcons->isChecked();
    m[QStringLiteral("show_shortcuts_in_context_menus")] = m_shortcuts->isChecked();
    m[QStringLiteral("activate_item_on_single_click")] = int(m_singleClick->checkState());
    m[QStringLiteral("dialog_buttons_have_icons")] = int(m_dialogIcons->checkState());
    m[QStringLiteral("underline_shortcut")] = int(m_underline->checkState());
    m[QStringLiteral("effects")] = m_effects->isChecked();
    m[QStringLiteral("menu_effect")] = m_menuEffect->currentIndex();
    m[QStringLiteral("combo_effect")] = m_comboEffect->currentIndex();
    m[QStringLiteral("tooltip_effect")] = m_tooltipEffect->currentIndex();
    m[QStringLiteral("toolbox_effect")] = m_toolboxEffect->currentIndex();
    return m;
}

QList<IniFile::Update> QtInterfacePage::updatesFor(QtTarget) const
{
    const QString sec = QStringLiteral("Interface");
    QStringList effects;
    if (m_effects->isChecked())
        effects << QStringLiteral("General");
    if (m_menuEffect->currentIndex() == 1)
        effects << QStringLiteral("AnimateMenu");
    else if (m_menuEffect->currentIndex() == 2)
        effects << QStringLiteral("FadeMenu");
    if (m_comboEffect->currentIndex() == 1)
        effects << QStringLiteral("AnimateCombo");
    if (m_tooltipEffect->currentIndex() == 1)
        effects << QStringLiteral("AnimateTooltip");
    else if (m_tooltipEffect->currentIndex() == 2)
        effects << QStringLiteral("FadeTooltip");
    if (m_toolboxEffect->currentIndex() == 1)
        effects << QStringLiteral("AnimateToolBox");

    return {QtCt::setInt(sec, QStringLiteral("double_click_interval"), m_doubleClick->value()),
            QtCt::setInt(sec, QStringLiteral("cursor_flash_time"), m_flash->value()),
            QtCt::setInt(sec, QStringLiteral("buttonbox_layout"), m_buttonLayout->currentData().toInt()),
            QtCt::setInt(sec, QStringLiteral("keyboard_scheme"), m_keyboard->currentData().toInt()),
            QtCt::setBool(sec, QStringLiteral("menus_have_icons"), m_menuIcons->isChecked()),
            QtCt::setBool(sec, QStringLiteral("show_shortcuts_in_context_menus"), m_shortcuts->isChecked()),
            QtCt::setInt(sec, QStringLiteral("underline_shortcut"), int(m_underline->checkState())),
            QtCt::setInt(sec, QStringLiteral("activate_item_on_single_click"), int(m_singleClick->checkState())),
            QtCt::setInt(sec, QStringLiteral("dialog_buttons_have_icons"), int(m_dialogIcons->checkState())),
            QtCt::setInt(sec, QStringLiteral("toolbutton_style"), m_toolButton->currentData().toInt()),
            QtCt::setInt(sec, QStringLiteral("wheel_scroll_lines"), m_wheel->value()),
            QtCt::setList(sec, QStringLiteral("gui_effects"), effects)};
}
