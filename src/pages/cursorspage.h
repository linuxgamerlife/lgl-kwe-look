#pragma once
#include "pagebase.h"

class AppModel;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;

// Cursor theme and size, decoded natively from the theme's Xcursor files.
class CursorsPage : public PageBase
{
    Q_OBJECT
public:
    explicit CursorsPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;
    bool wantsScrollArea() const override { return false; }

private:
    void populate();
    void filter(const QString &text);
    void updatePreview();

    AppModel *m_model;
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    QSpinBox *m_size = nullptr;
    QComboBox *m_preset = nullptr;
    QHBoxLayout *m_strip = nullptr;
    QLabel *m_title = nullptr;
};
