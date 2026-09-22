#pragma once
#include "qtpagebase.h"

class AppModel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

// Style sheets applied to every Qt app, in list order. Files are created, edited,
// renamed and removed immediately; which ones are enabled and in what order is
// part of Apply.
class QtStyleSheetsPage : public QtPageBase
{
    Q_OBJECT
public:
    QtStyleSheetsPage(QtTarget target, AppModel *model, QWidget *parent = nullptr);

    void load() override;
    bool wantsScrollArea() const override { return false; }

protected:
    QVariantMap snapshot() const override;
    QList<IniFile::Update> updatesFor(QtTarget target) const override;
    QString pageName() const override { return tr("style sheets"); }

private:
    QStringList enabledPaths() const;
    QString currentPath() const;
    bool isUserSheet(const QString &path) const;
    QString askName(const QString &title, const QString &suggestion);
    void addItem(const QString &path, bool checked);
    void updateButtons();
    void createSheet();
    void editSheet();
    void copySheet();
    void renameSheet();
    void removeSheet();

    AppModel *m_model;
    QListWidget *m_list = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_copy = nullptr;
    QPushButton *m_rename = nullptr;
    QPushButton *m_remove = nullptr;
};
