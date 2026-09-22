#include "iconspage.h"

#include "appmodel.h"
#include "core/themescan.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSplitter>

namespace {
const char *const kPreviewIcons[] = {"folder", "user-home", "text-x-generic", "applications-internet",
                                     "preferences-system", "system-file-manager", "utilities-terminal",
                                     "audio-x-generic", "image-x-generic", "video-x-generic",
                                     "edit-delete", "document-open"};
constexpr int kPreviewSize = 48;
constexpr int kColumns = 6;

// Small action glyphs. The first three sit beside each theme name in the list (the same three
// qt5ct and qt6ct show); all of them are shown for the selected theme.
const char *const kListGlyphs[] = {"document-save", "document-print", "media-playback-stop"};
const char *const kPreviewGlyphs[] = {"document-save", "document-print", "edit-copy", "edit-paste",
                                      "edit-find", "list-add", "go-next", "media-playback-start",
                                      "media-playback-stop", "dialog-ok", "dialog-warning", "view-refresh"};
constexpr int kGlyphSize = 24;
constexpr int kGlyphGap = 4;
constexpr int kListGlyphCount = 3;   // entries of kListGlyphs
}  // namespace

IconsPage::IconsPage(AppModel *model, QWidget *parent) : PageBase(parent), m_model(model)
{
    auto *root = initPage(tr("Icon Theme"),
                          tr("Shared by GTK, Qt5 and Qt6. One choice is written to every place that "
                             "reads it."));

    auto *split = new GripSplitter(Qt::Horizontal, this);

    auto *left = new QWidget(split);
    auto *ll = new QVBoxLayout(left);
    ll->setContentsMargins(0, 0, 0, 0);
    m_search = new QLineEdit(left);
    m_search->setPlaceholderText(tr("Search icon themes"));
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(left);
    ll->addWidget(withHelp(m_search, tr("Choose the icon theme for GTK, Qt5 and Qt6 apps and, in a KineticWE or KDE "
                                        "session, the desktop's own icon setting. The three small icons beside each "
                                        "name (save, print, stop) give a quick look at the theme.")));
    ll->addWidget(m_list, 1);
    m_left = left;
    m_list->setIconSize(QSize(kListGlyphCount * kGlyphSize + (kListGlyphCount - 1) * kGlyphGap, kGlyphSize));

    auto *right = new QWidget(split);
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(right, &pl);
    m_title = new QLabel(panel);
    pl->addWidget(m_title);
    auto *gridHost = new QWidget(panel);
    m_grid = new QGridLayout(gridHost);
    m_grid->setSpacing(10);
    pl->addWidget(gridHost);
    pl->addWidget(makeSectionTitle(panel, tr("Glyphs")));
    auto *glyphHost = new QWidget(panel);
    m_glyphRow = new QHBoxLayout(glyphHost);
    m_glyphRow->setContentsMargins(0, 0, 0, 0);
    m_glyphRow->setSpacing(10);
    pl->addWidget(glyphHost);
    rl->addWidget(panel);
    m_note = new QLabel(right);
    m_note->setWordWrap(true);
    rl->addWidget(m_note);
    rl->addStretch();

    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);
    root->addWidget(split, 1);

    connect(m_search, &QLineEdit::textChanged, this, &IconsPage::filter);
    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *cur) {
        if (!cur)
            return;
        m_model->state.iconTheme = cur->data(Qt::UserRole).toString();
        m_model->touch();
        updatePreview();
    });
    connect(m_model, &AppModel::reloaded, this, &IconsPage::load);

    load();
}

void IconsPage::load()
{
    m_glyphCache.clear();   // the themes on disk may have changed
    populate();
}

void IconsPage::populate()
{
    const QSignalBlocker block(m_list);
    m_list->clear();
    QListWidgetItem *current = nullptr;
    for (const ThemeScan::IconTheme &t : std::as_const(m_model->iconThemes)) {
        auto *it = new QListWidgetItem(glyphStrip(t.folder), t.displayName, m_list);
        it->setData(Qt::UserRole, t.folder);
        it->setToolTip(t.path);
        if (t.folder == m_model->state.iconTheme)
            current = it;
    }
    if (current)
        m_list->setCurrentItem(current);
    fitList();
    filter(m_search->text());
    updatePreview();
}

void IconsPage::filter(const QString &text)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        it->setHidden(!text.isEmpty() && !it->text().contains(text, Qt::CaseInsensitive));
    }
    if (m_list->currentItem())
        m_list->scrollToItem(m_list->currentItem());
}

// The list is as wide as its longest name plus the glyphs, so no theme name is cut off.
void IconsPage::fitList()
{
    const int width = m_list->sizeHintForColumn(0) + m_list->verticalScrollBar()->sizeHint().width()
                      + 2 * m_list->frameWidth() + 8;
    m_left->setMinimumWidth(qBound(240, width, 520));
}

QString IconsPage::resolveIcon(const QString &folder, const QString &name, int size, int depth) const
{
    for (const ThemeScan::IconTheme &t : m_model->iconThemes) {
        if (t.folder != folder)
            continue;
        const QString file = ThemeScan::findIconFile(t.path, name, size);
        if (!file.isEmpty() || depth >= 3)
            return file;
        for (const QString &parent : t.inherits) {
            const QString inherited = resolveIcon(parent, name, size, depth + 1);
            if (!inherited.isEmpty())
                return inherited;
        }
        return QString();
    }
    return QString();
}

QPixmap IconsPage::iconPixmap(const QString &folder, const QString &name, int size) const
{
    const QString file = resolveIcon(folder, name, size);
    return file.isEmpty() ? QPixmap() : QIcon(file).pixmap(QSize(size, size), devicePixelRatioF());
}

// The glyphs of a theme drawn side by side into one icon, so a plain list row can show them.
QIcon IconsPage::glyphStrip(const QString &folder)
{
    const auto cached = m_glyphCache.constFind(folder);
    if (cached != m_glyphCache.constEnd())
        return cached.value();

    const qreal dpr = devicePixelRatioF();
    const int width = kListGlyphCount * kGlyphSize + (kListGlyphCount - 1) * kGlyphGap;
    QPixmap strip(QSize(width, kGlyphSize) * dpr);
    strip.setDevicePixelRatio(dpr);
    strip.fill(Qt::transparent);
    {
        QPainter painter(&strip);
        int x = 0;
        for (const char *glyph : kListGlyphs) {
            const QPixmap pm = iconPixmap(folder, QLatin1String(glyph), kGlyphSize);
            if (!pm.isNull())
                painter.drawPixmap(x, 0, pm);
            x += kGlyphSize + kGlyphGap;
        }
    }
    const QIcon icon(strip);
    m_glyphCache.insert(folder, icon);
    return icon;
}

void IconsPage::updatePreview()
{
    while (QLayoutItem *item = m_grid->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QString folder = m_model->state.iconTheme;
    m_title->setText(tr("<b>Preview:</b> %1").arg(folder.toHtmlEscaped()));

    int i = 0;
    for (const char *icon : kPreviewIcons) {
        const QString name = QLatin1String(icon);
        const QString file = resolveIcon(folder, name, kPreviewSize);

        auto *cell = new QWidget;
        auto *cl = new QVBoxLayout(cell);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(2);

        auto *pic = new QLabel(cell);
        pic->setAlignment(Qt::AlignCenter);
        pic->setMinimumSize(kPreviewSize + 8, kPreviewSize + 8);
        if (!file.isEmpty()) {
            const QPixmap pm = QIcon(file).pixmap(kPreviewSize, kPreviewSize);
            if (!pm.isNull())
                pic->setPixmap(pm);
            else
                pic->setText(tr("?"));   // SVG theme but the Qt SVG plugin is missing
        } else {
            pic->setText(tr("–"));
        }
        auto *cap = new QLabel(name, cell);
        cap->setAlignment(Qt::AlignCenter);
        QFont f = cap->font();
        f.setPointSize(qMax(7, f.pointSize() - 3));
        cap->setFont(f);
        cl->addWidget(pic);
        cl->addWidget(cap);
        m_grid->addWidget(cell, i / kColumns, i % kColumns);
        ++i;
    }

    while (QLayoutItem *item = m_glyphRow->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (const char *glyph : kPreviewGlyphs) {
        const QString name = QLatin1String(glyph);
        auto *pic = new QLabel;
        pic->setToolTip(name);
        pic->setFixedSize(kGlyphSize + 4, kGlyphSize + 4);
        pic->setAlignment(Qt::AlignCenter);
        const QPixmap pm = iconPixmap(folder, name, kGlyphSize);
        if (!pm.isNull())
            pic->setPixmap(pm);
        else
            pic->setText(tr("–"));
        m_glyphRow->addWidget(pic);
    }
    m_glyphRow->addStretch();

    m_note->setText(tr("Applying writes appearance.kwe, kdeglobals, qt5ct.conf and qt6ct.conf (icon_theme) and "
                       "gsettings, then notifies running Qt and KDE apps. SVG themes need the qt6-qtsvg package "
                       "to show here."));
}
