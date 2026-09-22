#include "paletteeditdialog.h"

#include "pagehelpers.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// Same order as QPalette::ColorRole. Index 17 (NoRole) is not editable.
const char *const kRoleNames[] = {
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Window text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Button"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Light"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Midlight"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Dark"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Mid"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Bright text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Button text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Base"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Window"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Shadow"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Highlight"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Highlighted text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Link"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Visited link"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Alternate base"),
    nullptr,
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Tooltip base"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Tooltip text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Placeholder text"),
    QT_TRANSLATE_NOOP("PaletteEditDialog", "Accent"),
};
constexpr int kKnownRoles = int(sizeof(kRoleNames) / sizeof(kRoleNames[0]));

}  // namespace

PaletteEditDialog::PaletteEditDialog(const QtCt::Scheme &scheme, QWidget *parent)
    : QDialog(parent), m_scheme(scheme)
{
    setWindowTitle(tr("Edit Palette"));
    resize(560, 640);

    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(m_scheme.size(), 3, this);
    m_table->setHorizontalHeaderLabels({tr("Active"), tr("Inactive"), tr("Disabled")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList rowNames;
    for (int i = 0; i < m_scheme.size(); ++i) {
        rowNames << (i < kKnownRoles && kRoleNames[i] ? tr(kRoleNames[i]) : tr("Role %1").arg(i));
        for (int c = 0; c < 3; ++c)
            m_table->setItem(i, c, new QTableWidgetItem);
    }
    m_table->setVerticalHeaderLabels(rowNames);
    for (int i = 0; i < m_scheme.size(); ++i) {
        for (int c = 0; c < 3; ++c)
            refreshCell(i, c);
    }
    layout->addWidget(m_table);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    addSizeGrip(this);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &PaletteEditDialog::editCell);
}

void PaletteEditDialog::refreshCell(int row, int col)
{
    const QList<QColor> &group = col == 0 ? m_scheme.active : col == 1 ? m_scheme.inactive : m_scheme.disabled;
    const QColor c = group.at(row);
    QTableWidgetItem *it = m_table->item(row, col);
    it->setText(c.name(QColor::HexArgb));
    it->setBackground(c);
    // Keep the hex text readable on any swatch.
    it->setForeground(c.lightness() > 128 ? Qt::black : Qt::white);
}

void PaletteEditDialog::editCell(int row, int col)
{
    QList<QColor> &group = col == 0 ? m_scheme.active : col == 1 ? m_scheme.inactive : m_scheme.disabled;
    const QColor chosen = QColorDialog::getColor(group.at(row), this, tr("Choose Colour"),
                                                 QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid())
        return;
    group[row] = chosen;
    refreshCell(row, col);
    emit schemeChanged(m_scheme);
}
