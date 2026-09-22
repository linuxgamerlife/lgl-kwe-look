#include "qttroubleshootingpage.h"

#include "appmodel.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

QtTroubleshootingPage::QtTroubleshootingPage(QtTarget target, AppModel *model, QWidget *parent)
    : QtPageBase(target, parent), m_model(model)
{
    auto *root = initPage(tr("%1 Troubleshooting").arg(QtCt::majorName(target)),
                          tr("Workarounds for %1 apps that misbehave under the platform theme.")
                              .arg(QtCt::majorName(target)));

    QVBoxLayout *al = nullptr;
    QFrame *apps = makePanel(this, &al);
    al->addWidget(makeSectionTitle(apps, tr("Ignored applications")));
    al->addWidget(makeDescLabel(apps, tr("The platform theme does nothing inside these programs.")));
    auto *row = new QHBoxLayout;
    m_apps = new QListWidget(apps);
    row->addWidget(m_apps, 1);
    auto *col = new QVBoxLayout;
    auto *add = makeToolbarBtn(tr("Add…"), apps);
    auto *remove = makeToolbarBtn(tr("Remove"), apps);
    col->addWidget(makeHelpButton(apps, tr("Programs listed here are left alone by the platform theme: it does "
                                           "not change their style, colours or fonts. Add an app that looks "
                                           "wrong or crashes under the theme.")),
                   0, Qt::AlignRight);
    col->addWidget(add);
    col->addWidget(remove);
    col->addStretch();
    row->addLayout(col);
    al->addLayout(row);
    root->addWidget(apps, 1);

    QVBoxLayout *rl = nullptr;
    QFrame *raster = makePanel(this, &rl);
    m_raster = new QCheckBox(tr("Force raster widgets"), raster);
    m_raster->setTristate(true);
    rl->addWidget(withHelp(m_raster, tr("Draws widgets in software instead of through the GPU-accelerated path. "
                                        "It can fix flicker, missing or garbled controls in some apps.")));
    rl->addWidget(makeDescLabel(raster, tr("Works around rendering glitches in some widgets. The middle state "
                                           "leaves the application's own default.")));
    root->addWidget(raster);

    connect(add, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("Select Application"), QStringLiteral("/usr/bin"),
                                                          tr("Executable files (*)"));
        if (path.isEmpty())
            return;
        if (m_apps->findItems(path, Qt::MatchExactly).isEmpty())
            m_apps->addItem(path);
        edited();
    });
    connect(remove, &QPushButton::clicked, this, [this]() {
        delete m_apps->takeItem(m_apps->currentRow());
        edited();
    });
    connect(m_raster, &QAbstractButton::clicked, this, [this]() { edited(); });

    load();
}

void QtTroubleshootingPage::load()
{
    QSettings s(QtCt::configFile(m_target), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Troubleshooting"));
    m_apps->clear();
    m_apps->addItems(s.value(QStringLiteral("ignored_applications")).toStringList());
    const QSignalBlocker b(m_raster);
    m_raster->setCheckState(Qt::CheckState(s.value(QStringLiteral("force_raster_widgets"),
                                                   int(Qt::PartiallyChecked)).toInt()));
    s.endGroup();
    loaded();
}

QStringList QtTroubleshootingPage::ignored() const
{
    QStringList out;
    for (int i = 0; i < m_apps->count(); ++i)
        out << m_apps->item(i)->text();
    return out;
}

QVariantMap QtTroubleshootingPage::snapshot() const
{
    return {{QStringLiteral("ignored"), ignored()}, {QStringLiteral("raster"), int(m_raster->checkState())}};
}

QList<IniFile::Update> QtTroubleshootingPage::updatesFor(QtTarget) const
{
    const QString sec = QStringLiteral("Troubleshooting");
    return {QtCt::setList(sec, QStringLiteral("ignored_applications"), ignored()),
            QtCt::setInt(sec, QStringLiteral("force_raster_widgets"), int(m_raster->checkState()))};
}
