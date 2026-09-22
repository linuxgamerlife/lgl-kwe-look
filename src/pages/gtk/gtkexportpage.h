#pragma once
#include "../pagebase.h"
#include "gtkbindings.h"

class QCheckBox;
class QLabel;

// Which files are exported when GTK settings are applied, plus the Flatpak options.
class GtkExportPage : public PageBase
{
    Q_OBJECT
public:
    explicit GtkExportPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;

signals:
    void exportNowRequested();

private:
    void updateGtk4Note();

    AppModel *m_model;
    Bindings m_bind;
    QLabel *m_gtk4Note = nullptr;
};
