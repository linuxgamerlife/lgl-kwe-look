#include "qtfontspage.h"

#include "appmodel.h"

#include <QFontDatabase>
#include <QFontDialog>
#include <QLabel>
#include <QSettings>

QtFontsPage::QtFontsPage(QtTarget target, AppModel *model, QWidget *parent)
    : QtPageBase(target, parent), m_model(model)
{
    auto *root = initPage(tr("%1 Fonts").arg(QtCt::majorName(target)),
                          tr("Fonts for %1 apps. Applied by the %2 platform theme.")
                              .arg(QtCt::majorName(target), QtCt::toolName(target)));

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(this, &pl);

    const auto addRow = [&](const QString &title, const QString &help, QFont *font, QLabel **label) {
        pl->addWidget(makeSectionTitle(panel, title));
        auto *row = new QHBoxLayout;
        *label = new QLabel(panel);
        auto *btn = makeToolbarBtn(tr("Change…"), panel);
        row->addWidget(*label, 1);
        row->addWidget(btn);
        row->addWidget(makeHelpButton(panel, help));
        pl->addLayout(row);
        QLabel *l = *label;
        connect(btn, &QPushButton::clicked, this, [this, font, l]() { choose(font, l); });
    };
    addRow(tr("General"),
           tr("The font Qt apps use for menus, buttons and other interface text. Press Change to pick a family, "
              "style and size."),
           &m_general, &m_generalLabel);
    addRow(tr("Fixed width"),
           tr("The font Qt apps use where every letter must have the same width, such as in terminals and code "
              "editors."),
           &m_fixed, &m_fixedLabel);
    pl->addWidget(makeDescLabel(panel, tr("The fixed-width font is used by terminals and code views in Qt apps.")));

    root->addWidget(panel);
    root->addStretch();

    load();
}

void QtFontsPage::load()
{
    QSettings s(QtCt::configFile(m_target), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Fonts"));
    // QFont::fromString reads both the Qt5 and the Qt6 format.
    m_general = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    m_general.fromString(s.value(QStringLiteral("general"), m_general.toString()).toString());
    m_fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_fixed.fromString(s.value(QStringLiteral("fixed"), m_fixed.toString()).toString());
    s.endGroup();

    show(m_general, m_generalLabel);
    show(m_fixed, m_fixedLabel);
    loaded();
}

void QtFontsPage::show(const QFont &font, QLabel *label)
{
    label->setText(QStringLiteral("%1 %2").arg(font.family()).arg(font.pointSizeF() > 0 ? font.pointSizeF() : 10.0));
    label->setFont(font);
}

void QtFontsPage::choose(QFont *font, QLabel *label)
{
    bool ok = false;
    const QFont f = pickFont(this, *font, &ok);
    if (!ok)
        return;
    *font = f;
    show(*font, label);
    edited();
}

QVariantMap QtFontsPage::snapshot() const
{
    // Normalised through the Qt6 format so equal fonts compare equal.
    return {{QStringLiteral("general"), m_general.toString()}, {QStringLiteral("fixed"), m_fixed.toString()}};
}

QList<IniFile::Update> QtFontsPage::updatesFor(QtTarget target) const
{
    return {QtCt::setString(QStringLiteral("Fonts"), QStringLiteral("general"),
                            QtCt::fontToConfigString(m_general, target)),
            QtCt::setString(QStringLiteral("Fonts"), QStringLiteral("fixed"),
                            QtCt::fontToConfigString(m_fixed, target))};
}
