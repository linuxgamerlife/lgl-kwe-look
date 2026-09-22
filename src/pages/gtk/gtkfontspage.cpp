#include "gtkfontspage.h"

#include "appmodel.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QLineEdit>

namespace {

// GTK font description: "Family [Bold] [Italic] Size"
QString gtkFontString(const QFont &f)
{
    QString s = f.family();
    if (f.bold())
        s += QStringLiteral(" Bold");
    if (f.italic())
        s += QStringLiteral(" Italic");
    return s + QLatin1Char(' ') + QString::number(f.pointSizeF() > 0 ? f.pointSizeF() : 10.0, 'g', 4);
}

QFont qtFontFromGtk(const QString &desc)
{
    // Best effort: trailing number is the size, the rest is the family and style words.
    QStringList words = desc.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QFont f;
    if (words.isEmpty())
        return f;
    bool ok = false;
    const double size = words.last().toDouble(&ok);
    if (ok) {
        f.setPointSizeF(size);
        words.removeLast();
    }
    while (!words.isEmpty()) {
        const QString w = words.last().toLower();
        if (w == QLatin1String("bold"))        f.setBold(true);
        else if (w == QLatin1String("italic")) f.setItalic(true);
        else break;
        words.removeLast();
    }
    f.setFamily(words.join(QLatin1Char(' ')));
    return f;
}

}  // namespace

GtkFontsPage::GtkFontsPage(AppModel *model, QWidget *parent)
    : PageBase(parent), m_model(model), m_bind(model)
{
    auto *root = initPage(tr("GTK Fonts"), tr("Font and text rendering for GTK apps."));

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(this, &pl);
    auto *form = new QFormLayout;

    auto *fontRow = new QHBoxLayout;
    m_font = new QLineEdit(panel);
    auto *pick = makeToolbarBtn(tr("Choose…"), panel);
    fontRow->addWidget(m_font, 1);
    fontRow->addWidget(pick);
    fontRow->addWidget(makeHelpButton(panel, tr("The default font for the interface text of GTK apps, written as "
                                                "family, style and size, for example \"Noto Sans 11\". Press "
                                                "Choose to pick it from a list.")));
    form->addRow(tr("Font"), fontRow);

    m_scale = new QDoubleSpinBox(panel);
    m_scale->setRange(0.5, 3.0);
    m_scale->setSingleStep(0.05);
    m_scale->setDecimals(2);
    form->addRow(tr("Text scaling factor"),
                 withHelp(m_scale, tr("Multiplies the size of all text in GTK apps. 1.00 is normal and 1.25 makes "
                                      "text a quarter larger. It scales text only, not icons or buttons.")));

    m_aa = new QComboBox(panel);
    m_aa->addItem(tr("None"), QStringLiteral("none"));
    m_aa->addItem(tr("Grayscale"), QStringLiteral("grayscale"));
    m_aa->addItem(tr("Subpixel (rgba)"), QStringLiteral("rgba"));
    form->addRow(tr("Antialiasing"),
                 withHelp(m_aa, tr("Smooths the edges of letters. Grayscale works on every screen. Subpixel uses the "
                                   "colour stripes of an LCD panel for sharper text. None gives jagged edges.")));

    m_hinting = new QComboBox(panel);
    m_hinting->addItem(tr("None"), QStringLiteral("none"));
    m_hinting->addItem(tr("Slight"), QStringLiteral("slight"));
    m_hinting->addItem(tr("Medium"), QStringLiteral("medium"));
    m_hinting->addItem(tr("Full"), QStringLiteral("full"));
    form->addRow(tr("Hinting"),
                 withHelp(m_hinting, tr("How strongly letter shapes are snapped to the pixel grid. Slight is a good "
                                        "default. Full is crisp but bends the shapes, and None keeps them exact but "
                                        "can look soft on low-resolution screens.")));

    m_rgba = new QComboBox(panel);
    m_rgba->addItem(QStringLiteral("RGB"), QStringLiteral("rgb"));
    m_rgba->addItem(QStringLiteral("BGR"), QStringLiteral("bgr"));
    m_rgba->addItem(QStringLiteral("VRGB"), QStringLiteral("vrgb"));
    m_rgba->addItem(QStringLiteral("VBGR"), QStringLiteral("vbgr"));
    m_rgba->addItem(tr("None"), QStringLiteral("none"));
    form->addRow(tr("Subpixel order"),
                 withHelp(m_rgba, tr("The order of the red, green and blue stripes in your screen. RGB is right for "
                                     "most panels. It only matters with Subpixel antialiasing.")));

    pl->addLayout(form);
    root->addWidget(panel);
    root->addStretch();

    m_bind.combo(m_aa, &ThemeState::fontAntialiasing);
    m_bind.combo(m_hinting, &ThemeState::fontHinting);
    m_bind.combo(m_rgba, &ThemeState::fontRgbaOrder);

    connect(pick, &QPushButton::clicked, this, &GtkFontsPage::chooseFont);
    connect(m_font, &QLineEdit::editingFinished, this, [this]() {
        m_model->state.fontName = m_font->text().trimmed();
        m_model->touch();
    });
    connect(m_scale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        m_model->state.textScalingFactor = v;
        m_model->touch();
    });
    connect(m_model, &AppModel::reloaded, this, &GtkFontsPage::load);

    load();
}

void GtkFontsPage::load()
{
    m_bind.refresh();
    {
        const QSignalBlocker b(m_font);
        m_font->setText(m_model->state.fontName);
    }
    {
        const QSignalBlocker b(m_scale);
        m_scale->setValue(m_model->state.textScalingFactor);
    }
}

void GtkFontsPage::chooseFont()
{
    bool ok = false;
    const QFont f = pickFont(this, qtFontFromGtk(m_model->state.fontName), &ok);
    if (!ok)
        return;
    m_model->state.fontName = gtkFontString(f);
    m_font->setText(m_model->state.fontName);
    m_model->touch();
}
