#pragma once
#include "../pagebase.h"
#include "gtkbindings.h"

class QCheckBox;
class QComboBox;

// Toolbar and sound settings, plus the deprecated settings.ini-only values.
class GtkOtherPage : public PageBase
{
    Q_OBJECT
public:
    explicit GtkOtherPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;

private:
    AppModel *m_model;
    Bindings m_bind;
};
