#pragma once
#include <QDialog>

#include "core/qtct.h"

class QTableWidget;
class QTableWidgetItem;

// Edits every colour role of a scheme in the three colour groups.
class PaletteEditDialog : public QDialog
{
    Q_OBJECT
public:
    PaletteEditDialog(const QtCt::Scheme &scheme, QWidget *parent = nullptr);

    QtCt::Scheme scheme() const { return m_scheme; }

signals:
    void schemeChanged(const QtCt::Scheme &scheme);

private:
    void refreshCell(int row, int col);
    void editCell(int row, int col);

    QtCt::Scheme m_scheme;
    QTableWidget *m_table = nullptr;
};
