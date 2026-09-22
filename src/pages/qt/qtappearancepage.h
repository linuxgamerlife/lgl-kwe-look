#pragma once
#include <QPalette>

#include "qtpagebase.h"

class AppModel;
class PreviewPanel;
class QAction;
class QComboBox;
class QFileSystemWatcher;
class QLabel;
class QPushButton;
class QStyle;
class QTimer;

// Style, colour scheme and standard dialogs for one Qt major.
class QtAppearancePage : public QtPageBase
{
    Q_OBJECT
public:
    QtAppearancePage(QtTarget target, AppModel *model, QWidget *parent = nullptr);

    void load() override;
    void addToPlan(ApplyPlan &plan, bool linked) override;
    bool wantsScrollArea() const override { return false; }

protected:
    QVariantMap snapshot() const override;
    QList<IniFile::Update> updatesFor(QtTarget target) const override;
    QString pageName() const override { return tr("appearance"); }

private:
    void populateSchemes(const QString &selectedPath, bool custom);
    void applyPreview();
    void onSchemeChosen();
    void watchScheme();
    void reloadManagedScheme();
    void updateNotes();
    void createScheme();
    void editScheme();
    void copyScheme();
    void renameScheme();
    void removeScheme();
    QString schemeData() const;             // "system", "style" or a path
    int writeRoleCount(bool linked) const;
    QString newSchemePath(const QString &title, const QString &suggestion);

    AppModel *m_model;
    QComboBox *m_style = nullptr;
    QComboBox *m_scheme = nullptr;
    QComboBox *m_dialogs = nullptr;
    QComboBox *m_group = nullptr;
    QPushButton *m_schemeMenu = nullptr;
    QAction *m_edit = nullptr;
    QAction *m_rename = nullptr;
    QAction *m_remove = nullptr;
    QLabel *m_notes = nullptr;
    PreviewPanel *m_preview = nullptr;
    QStyle *m_previewStyle = nullptr;
    QFileSystemWatcher *m_watcher = nullptr;   // follows the session's live scheme (Noctalia)
    QTimer *m_reloadTimer = nullptr;
    QtCt::Scheme m_custom;    // palette of the selected file scheme, possibly edited live
    bool m_approximate = false;
};
