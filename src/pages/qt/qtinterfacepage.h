#pragma once
#include "qtpagebase.h"

class AppModel;
class QCheckBox;
class QComboBox;
class QSpinBox;

// Behaviour hints read by the platform theme: double click, effects, menus, keyboard.
class QtInterfacePage : public QtPageBase
{
    Q_OBJECT
public:
    QtInterfacePage(QtTarget target, AppModel *model, QWidget *parent = nullptr);

    void load() override;

protected:
    QVariantMap snapshot() const override;
    QList<IniFile::Update> updatesFor(QtTarget target) const override;
    QString pageName() const override { return tr("interface"); }

private:
    QCheckBox *triState(const QString &text, QWidget *parent);

    AppModel *m_model;
    QSpinBox *m_doubleClick = nullptr;
    QSpinBox *m_flash = nullptr;
    QSpinBox *m_wheel = nullptr;
    QComboBox *m_buttonLayout = nullptr;
    QComboBox *m_keyboard = nullptr;
    QComboBox *m_toolButton = nullptr;
    QCheckBox *m_menuIcons = nullptr;
    QCheckBox *m_shortcuts = nullptr;
    QCheckBox *m_singleClick = nullptr;
    QCheckBox *m_dialogIcons = nullptr;
    QCheckBox *m_underline = nullptr;
    QCheckBox *m_effects = nullptr;
    QComboBox *m_menuEffect = nullptr;
    QComboBox *m_comboEffect = nullptr;
    QComboBox *m_tooltipEffect = nullptr;
    QComboBox *m_toolboxEffect = nullptr;
};
