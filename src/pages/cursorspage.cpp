#include "cursorspage.h"

#include "appmodel.h"
#include "core/themescan.h"
#include "core/xcursor.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QSpinBox>
#include <QSplitter>

namespace {

struct CursorSlot {
    const char *label;
    QStringList names;   // first file that exists in the theme wins
};

const QList<CursorSlot> &cursorSlots()
{
    static const QList<CursorSlot> s{
        {QT_TRANSLATE_NOOP("CursorsPage", "Default"), {QStringLiteral("left_ptr"), QStringLiteral("default"), QStringLiteral("arrow")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Text"), {QStringLiteral("text"), QStringLiteral("xterm"), QStringLiteral("ibeam")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Link"), {QStringLiteral("pointer"), QStringLiteral("hand2"), QStringLiteral("hand1"),
                  QStringLiteral("pointing_hand")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Busy"), {QStringLiteral("wait"), QStringLiteral("watch"), QStringLiteral("progress")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Help"), {QStringLiteral("help"), QStringLiteral("question_arrow")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Precision"), {QStringLiteral("crosshair"), QStringLiteral("cross")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Grab"), {QStringLiteral("grabbing"), QStringLiteral("closedhand"), QStringLiteral("fleur")}},
        {QT_TRANSLATE_NOOP("CursorsPage", "Move"), {QStringLiteral("all-scroll"), QStringLiteral("size_all"), QStringLiteral("fleur")}}};
    return s;
}

const int kPresets[] = {16, 24, 32, 36, 48, 64, 96};

}  // namespace

CursorsPage::CursorsPage(AppModel *model, QWidget *parent) : PageBase(parent), m_model(model)
{
    auto *root = initPage(tr("Mouse Cursor"),
                          tr("Shared by GTK, Qt5, Qt6 and the compositor. The compositor reads the theme "
                             "from kineticwe.kwe and switches the pointer live."));

    auto *split = new GripSplitter(Qt::Horizontal, this);

    auto *left = new QWidget(split);
    auto *ll = new QVBoxLayout(left);
    ll->setContentsMargins(0, 0, 0, 0);
    m_search = new QLineEdit(left);
    m_search->setPlaceholderText(tr("Search cursor themes"));
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(left);
    ll->addWidget(withHelp(m_search, tr("Choose the mouse pointer theme. It is written for GTK, Qt and X11/XWayland "
                                        "apps and, in a KineticWE or KDE session, for the compositor or Plasma, "
                                        "which change the pointer without a restart.")));
    ll->addWidget(m_list, 1);

    auto *right = new QWidget(split);
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(right, &pl);
    m_title = new QLabel(panel);
    pl->addWidget(m_title);
    auto *strip = new QWidget(panel);
    m_strip = new QHBoxLayout(strip);
    m_strip->setSpacing(16);
    pl->addWidget(strip);
    rl->addWidget(panel);

    QVBoxLayout *sl = nullptr;
    QFrame *sizePanel = makePanel(right, &sl);
    sl->addWidget(makeSectionTitle(sizePanel, tr("Size")));
    auto *row = new QHBoxLayout;
    m_size = new QSpinBox(sizePanel);
    m_size->setRange(8, 128);
    m_size->setSuffix(tr(" px"));
    m_preset = new QComboBox(sizePanel);
    m_preset->addItem(tr("Presets…"), 0);
    for (int p : kPresets)
        m_preset->addItem(tr("%1 px").arg(p), p);
    row->addWidget(m_size);
    row->addWidget(m_preset);
    row->addWidget(makeHelpButton(sizePanel, tr("The pointer size in pixels. 24 is the usual default; larger values "
                                                "help on high-resolution screens. A preset fills in a common value.")));
    row->addStretch();
    sl->addLayout(row);
    rl->addWidget(sizePanel);
    rl->addStretch();

    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);
    root->addWidget(split, 1);

    connect(m_search, &QLineEdit::textChanged, this, &CursorsPage::filter);
    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *cur) {
        if (!cur)
            return;
        m_model->state.cursorTheme = cur->data(Qt::UserRole).toString();
        m_model->touch();
        updatePreview();
    });
    connect(m_size, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        m_model->state.cursorSize = v;
        m_model->touch();
        updatePreview();
    });
    connect(m_preset, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        const int v = m_preset->itemData(index).toInt();
        if (v > 0)
            m_size->setValue(v);
        m_preset->setCurrentIndex(0);
    });
    connect(m_model, &AppModel::reloaded, this, &CursorsPage::load);

    load();
}

void CursorsPage::load()
{
    populate();
}

void CursorsPage::populate()
{
    {
        const QSignalBlocker b(m_size);
        m_size->setValue(m_model->state.cursorSize);
    }
    const QSignalBlocker block(m_list);
    m_list->clear();
    QListWidgetItem *current = nullptr;
    for (const ThemeScan::CursorTheme &t : std::as_const(m_model->cursorThemes)) {
        auto *it = new QListWidgetItem(t.displayName == t.folder ? t.folder
                                                                  : QStringLiteral("%1  (%2)").arg(t.displayName, t.folder),
                                       m_list);
        it->setData(Qt::UserRole, t.folder);
        it->setToolTip(t.path);
        if (t.folder == m_model->state.cursorTheme)
            current = it;
    }
    if (current)
        m_list->setCurrentItem(current);
    filter(m_search->text());
    updatePreview();
}

void CursorsPage::filter(const QString &text)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        it->setHidden(!text.isEmpty() && !it->text().contains(text, Qt::CaseInsensitive));
    }
    if (m_list->currentItem())
        m_list->scrollToItem(m_list->currentItem());
}

void CursorsPage::updatePreview()
{
    while (QLayoutItem *item = m_strip->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QString folder = m_model->state.cursorTheme;
    const int size = m_model->state.cursorSize;
    m_title->setText(tr("<b>Preview:</b> %1 at %2 px").arg(folder.toHtmlEscaped()).arg(size));

    QString themePath;
    for (const ThemeScan::CursorTheme &t : m_model->cursorThemes) {
        if (t.folder == folder)
            themePath = t.path;
    }

    for (const CursorSlot &slot : cursorSlots()) {
        auto *cell = new QWidget;
        auto *cl = new QVBoxLayout(cell);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(2);

        auto *pic = new QLabel(cell);
        pic->setAlignment(Qt::AlignCenter);
        pic->setMinimumSize(size + 8, size + 8);

        bool shown = false;
        for (const QString &name : slot.names) {
            if (themePath.isEmpty())
                break;
            const auto frames = XCursor::decodeFile(themePath + QStringLiteral("/cursors/") + name);
            const XCursor::Frame f = XCursor::bestFrame(frames, size);
            if (f.image.isNull())
                continue;
            QImage img = f.image;
            if (f.nominalSize != size)
                img = img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            pic->setPixmap(QPixmap::fromImage(img));
            shown = true;
            break;
        }
        if (!shown)
            pic->setText(tr("–"));

        auto *cap = new QLabel(QCoreApplication::translate("CursorsPage", slot.label), cell);
        cap->setAlignment(Qt::AlignCenter);
        cl->addWidget(pic);
        cl->addWidget(cap);
        m_strip->addWidget(cell);
    }
    m_strip->addStretch();
}
