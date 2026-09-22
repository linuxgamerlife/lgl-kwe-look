#include "qtstylesheetspage.h"

#include "appmodel.h"
#include "qsseditordialog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSet>

QtStyleSheetsPage::QtStyleSheetsPage(QtTarget target, AppModel *model, QWidget *parent)
    : QtPageBase(target, parent), m_model(model)
{
    auto *root = initPage(tr("%1 Style Sheets").arg(QtCt::majorName(target)),
                          tr("Tick the sheets to apply to %1 apps. Drag to change the order, later sheets win.")
                              .arg(QtCt::majorName(target)));

    auto *row = new QHBoxLayout;
    m_list = new QListWidget(this);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);
    row->addWidget(m_list, 1);

    auto *col = new QVBoxLayout;
    col->addWidget(makeHelpButton(this, tr("Style sheets (QSS) restyle Qt apps with CSS-like rules, for example to "
                                           "round corners or change colours. A ticked sheet is applied to every "
                                           "Qt app, in list order. Drag to reorder: a later sheet wins over an "
                                           "earlier one.")),
                   0, Qt::AlignRight);
    auto *create = makeToolbarBtn(tr("Create"), this);
    m_edit = makeToolbarBtn(tr("Edit"), this);
    m_copy = makeToolbarBtn(tr("Create a Copy"), this);
    m_rename = makeToolbarBtn(tr("Rename"), this);
    m_remove = makeToolbarBtn(tr("Remove"), this);
    for (QPushButton *b : {create, m_edit, m_copy, m_rename, m_remove})
        col->addWidget(b);
    col->addStretch();
    row->addLayout(col);
    root->addLayout(row, 1);

    root->addWidget(makeDescLabel(this, tr("Sheets in your own folder can be edited, renamed and removed. "
                                           "Shared sheets are read-only; copy one to change it.")));

    connect(create, &QPushButton::clicked, this, &QtStyleSheetsPage::createSheet);
    connect(m_edit, &QPushButton::clicked, this, &QtStyleSheetsPage::editSheet);
    connect(m_copy, &QPushButton::clicked, this, &QtStyleSheetsPage::copySheet);
    connect(m_rename, &QPushButton::clicked, this, &QtStyleSheetsPage::renameSheet);
    connect(m_remove, &QPushButton::clicked, this, &QtStyleSheetsPage::removeSheet);
    connect(m_list, &QListWidget::currentRowChanged, this, [this]() { updateButtons(); });
    connect(m_list, &QListWidget::itemChanged, this, [this]() { edited(); });
    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, [this]() { edited(); });

    load();
}

void QtStyleSheetsPage::addItem(const QString &path, bool checked)
{
    auto *it = new QListWidgetItem(QFileInfo(path).completeBaseName(), m_list);
    it->setData(Qt::UserRole, path);
    it->setToolTip(path);
    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
    it->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void QtStyleSheetsPage::load()
{
    QSettings s(QtCt::configFile(m_target), QSettings::IniFormat);
    const QStringList enabled = s.value(QStringLiteral("Interface/stylesheets")).toStringList();

    const QSignalBlocker b(m_list);
    m_list->clear();
    QSet<QString> shown;

    // Enabled sheets first, in their stored order, even if the file has moved away
    // (shown with a warning tooltip so it can be unticked instead of silently dropped).
    for (const QString &p : enabled) {
        if (p.isEmpty() || shown.contains(p))
            continue;
        addItem(p, true);
        shown.insert(p);
        if (!QFileInfo::exists(p))
            m_list->item(m_list->count() - 1)->setToolTip(tr("%1 (file not found)").arg(p));
    }

    QDir().mkpath(QtCt::userStyleSheetsDir(m_target));
    QStringList dirs{QtCt::userStyleSheetsDir(m_target)};
    dirs << QtCt::sharedStyleSheetDirs(m_target);
    for (const QString &dir : std::as_const(dirs)) {
        const QDir d(dir);
        for (const QFileInfo &fi : d.entryInfoList(QStringList{QStringLiteral("*.qss")}, QDir::Files, QDir::Name)) {
            const QString p = fi.absoluteFilePath();
            if (shown.contains(p))
                continue;
            addItem(p, false);
            shown.insert(p);
        }
    }
    updateButtons();
    loaded();
}

QStringList QtStyleSheetsPage::enabledPaths() const
{
    QStringList out;
    for (int i = 0; i < m_list->count(); ++i) {
        const QListWidgetItem *it = m_list->item(i);
        if (it->checkState() == Qt::Checked)
            out << it->data(Qt::UserRole).toString();
    }
    return out;
}

QString QtStyleSheetsPage::currentPath() const
{
    const QListWidgetItem *it = m_list->currentItem();
    return it ? it->data(Qt::UserRole).toString() : QString();
}

bool QtStyleSheetsPage::isUserSheet(const QString &path) const
{
    return QFileInfo(path).absolutePath() == QDir(QtCt::userStyleSheetsDir(m_target)).absolutePath()
        && QFileInfo(path).isWritable();
}

void QtStyleSheetsPage::updateButtons()
{
    const QString p = currentPath();
    const bool any = !p.isEmpty();
    m_edit->setEnabled(any);
    m_edit->setText(any && !isUserSheet(p) ? tr("View") : tr("Edit"));
    m_copy->setEnabled(any);
    m_rename->setEnabled(any && isUserSheet(p));
    m_remove->setEnabled(any && isUserSheet(p));
}

QString QtStyleSheetsPage::askName(const QString &title, const QString &suggestion)
{
    bool ok = false;
    QString name = QInputDialog::getText(this, title, tr("File name:"), QLineEdit::Normal, suggestion, &ok).trimmed();
    if (!ok || name.isEmpty())
        return QString();
    if (name.contains(QLatin1Char('/')) || name.contains(QStringLiteral(".."))) {
        QMessageBox::warning(this, tr("Error"), tr("The name is not valid."));
        return QString();
    }
    if (!name.endsWith(QLatin1String(".qss"), Qt::CaseInsensitive))
        name += QStringLiteral(".qss");
    const QString path = QtCt::userStyleSheetsDir(m_target) + QLatin1Char('/') + name;
    if (QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Error"), tr("The style sheet \"%1\" already exists.").arg(name));
        return QString();
    }
    return path;
}

void QtStyleSheetsPage::createSheet()
{
    const QString path = askName(tr("Enter Style Sheet Name"), QString());
    if (path.isEmpty())
        return;
    QString err;
    if (!IniFile::writeTextAtomic(path, QString(), &err)) {
        QMessageBox::warning(this, tr("Error"), err);
        return;
    }
    addItem(path, false);
    m_list->setCurrentRow(m_list->count() - 1);
    editSheet();
}

void QtStyleSheetsPage::editSheet()
{
    const QString path = currentPath();
    if (path.isEmpty())
        return;
    const bool readOnly = !isUserSheet(path);
    QssEditorDialog d(path, readOnly, this);
    if (d.exec() != QDialog::Accepted || readOnly)
        return;
    QString err;
    if (!IniFile::writeTextAtomic(path, d.text(), &err))
        QMessageBox::warning(this, tr("Error"), err);
}

void QtStyleSheetsPage::copySheet()
{
    const QString src = currentPath();
    if (src.isEmpty())
        return;
    const QString path = askName(tr("Enter Style Sheet Name"), tr("%1 (copy)").arg(QFileInfo(src).completeBaseName()));
    if (path.isEmpty())
        return;
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!QFile::copy(src, path)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to copy file"));
        return;
    }
    addItem(path, false);
    m_list->setCurrentRow(m_list->count() - 1);
}

void QtStyleSheetsPage::renameSheet()
{
    QListWidgetItem *it = m_list->currentItem();
    if (!it || !isUserSheet(currentPath()))
        return;
    const QString src = currentPath();
    const QString path = askName(tr("Enter Style Sheet Name"), QFileInfo(src).completeBaseName());
    if (path.isEmpty() || !QFile::rename(src, path)) {
        return;
    }
    it->setText(QFileInfo(path).completeBaseName());
    it->setData(Qt::UserRole, path);
    it->setToolTip(path);
    edited();
}

void QtStyleSheetsPage::removeSheet()
{
    QListWidgetItem *it = m_list->currentItem();
    const QString path = currentPath();
    if (!it || !isUserSheet(path))
        return;
    if (QMessageBox::question(this, tr("Confirm Remove"),
                              tr("Are you sure you want to remove style sheet \"%1\"?").arg(it->text()),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    if (!QFile::remove(path))
        return;
    delete m_list->takeItem(m_list->row(it));
    edited();
}

QVariantMap QtStyleSheetsPage::snapshot() const
{
    return {{QStringLiteral("stylesheets"), enabledPaths()}};
}

QList<IniFile::Update> QtStyleSheetsPage::updatesFor(QtTarget) const
{
    return {QtCt::setList(QStringLiteral("Interface"), QStringLiteral("stylesheets"), enabledPaths())};
}
