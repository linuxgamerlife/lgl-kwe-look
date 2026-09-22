#include "previewpanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

PreviewPanel::PreviewPanel(QWidget *parent) : QWidget(parent)
{
    auto *grid = new QGridLayout(this);

    auto *buttons = new QGroupBox(tr("Buttons"), this);
    auto *bl = new QVBoxLayout(buttons);
    auto *push = new QPushButton(tr("Push button"), buttons);
    auto *def = new QPushButton(tr("Default button"), buttons);
    def->setDefault(true);
    auto *off = new QPushButton(tr("Disabled"), buttons);
    off->setEnabled(false);
    bl->addWidget(push);
    bl->addWidget(def);
    bl->addWidget(off);

    auto *choices = new QGroupBox(tr("Choices"), this);
    auto *cl = new QVBoxLayout(choices);
    auto *check = new QCheckBox(tr("Check box"), choices);
    check->setChecked(true);
    auto *unchecked = new QCheckBox(tr("Unchecked"), choices);
    auto *radioA = new QRadioButton(tr("Radio A"), choices);
    radioA->setChecked(true);
    auto *radioB = new QRadioButton(tr("Radio B"), choices);
    cl->addWidget(check);
    cl->addWidget(unchecked);
    cl->addWidget(radioA);
    cl->addWidget(radioB);

    auto *inputs = new QGroupBox(tr("Inputs"), this);
    auto *il = new QVBoxLayout(inputs);
    auto *edit = new QLineEdit(tr("Line edit"), inputs);
    auto *combo = new QComboBox(inputs);
    combo->addItems({tr("Combo box"), tr("Second item")});
    auto *spin = new QSpinBox(inputs);
    spin->setValue(42);
    il->addWidget(edit);
    il->addWidget(combo);
    il->addWidget(spin);

    auto *values = new QGroupBox(tr("Values"), this);
    auto *vl = new QVBoxLayout(values);
    auto *slider = new QSlider(Qt::Horizontal, values);
    slider->setValue(60);
    auto *progress = new QProgressBar(values);
    progress->setValue(40);
    auto *link = new QLabel(tr("<a href=\"#\">A link</a> and plain text"), values);
    vl->addWidget(slider);
    vl->addWidget(progress);
    vl->addWidget(link);

    auto *tabs = new QTabWidget(this);
    auto *list = new QListWidget(tabs);
    list->addItems({tr("List item one"), tr("List item two"), tr("List item three")});
    list->setCurrentRow(1);
    tabs->addTab(list, tr("List"));
    tabs->addTab(new QLabel(tr("Second page"), tabs), tr("Tab"));

    grid->addWidget(buttons, 0, 0);
    grid->addWidget(choices, 0, 1);
    grid->addWidget(inputs, 1, 0);
    grid->addWidget(values, 1, 1);
    grid->addWidget(tabs, 2, 0, 1, 2);
}

void PreviewPanel::applyStyle(QWidget *w, QStyle *style)
{
    for (QObject *o : w->children()) {
        if (o->isWidgetType())
            applyStyle(static_cast<QWidget *>(o), style);
    }
    w->setStyle(style);
}

void PreviewPanel::applyPalette(QWidget *w, const QPalette &palette)
{
    for (QObject *o : w->children()) {
        if (o->isWidgetType())
            applyPalette(static_cast<QWidget *>(o), palette);
    }
    w->setPalette(palette);
}

void PreviewPanel::setPreviewStyle(QStyle *style)
{
    applyStyle(this, style);
}

void PreviewPanel::setPreviewPalette(const QPalette &palette, QPalette::ColorGroup group)
{
    QPalette p = this->palette();
    for (int i = 0; i < QPalette::NColorRoles; ++i) {
        const auto role = QPalette::ColorRole(i);
        p.setColor(QPalette::Active, role, palette.color(group, role));
        p.setColor(QPalette::Inactive, role, palette.color(group, role));
        // Disabled widgets always show the scheme's Disabled colours, so a live update reaches them.
        p.setColor(QPalette::Disabled, role, palette.color(QPalette::Disabled, role));
    }
    applyPalette(this, p);
}

void PreviewPanel::setPreviewStyleSheet(const QString &sheet)
{
    setStyleSheet(sheet);
}
