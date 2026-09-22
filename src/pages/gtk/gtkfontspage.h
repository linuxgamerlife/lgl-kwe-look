#pragma once
#include "../pagebase.h"
#include "gtkbindings.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;

// GTK font and text rendering (gsettings font-name, text-scaling-factor,
// font-antialiasing, font-hinting, font-rgba-order).
class GtkFontsPage : public PageBase
{
    Q_OBJECT
public:
    explicit GtkFontsPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;

private:
    void chooseFont();

    AppModel *m_model;
    Bindings m_bind;
    QLineEdit *m_font = nullptr;
    QDoubleSpinBox *m_scale = nullptr;
    QComboBox *m_aa = nullptr;
    QComboBox *m_hinting = nullptr;
    QComboBox *m_rgba = nullptr;
};
