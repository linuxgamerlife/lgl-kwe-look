#include "gtkthemepage.h"

#include "appmodel.h"
#include "core/kwepaths.h"
#include "gtkpreviewclient.h"

#include <QComboBox>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QSplitter>
#include <QTimer>

GtkThemePage::GtkThemePage(AppModel *model, QWidget *parent)
    : PageBase(parent), m_model(model), m_bind(model)
{
    auto *root = initPage(tr("GTK Theme"),
                          tr("The widget theme for GTK3 and GTK4 apps, written to gsettings and settings.ini."));

    auto *split = new GripSplitter(Qt::Horizontal, this);

    auto *left = new QWidget(split);
    auto *ll = new QVBoxLayout(left);
    ll->setContentsMargins(0, 0, 0, 0);
    m_search = new QLineEdit(left);
    m_search->setPlaceholderText(tr("Search GTK themes"));
    m_search->setClearButtonEnabled(true);
    m_list = new QListWidget(left);
    ll->addWidget(withHelp(m_search, tr("Choose the widget theme GTK3 and GTK4 apps use. Themes are read from "
                                        "~/.themes, ~/.local/share/themes and /usr/share/themes. The choice is "
                                        "written when you press Apply.")));
    ll->addWidget(m_list, 1);

    auto *right = new QWidget(split);
    auto *rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(right, &pl);
    auto *previewHead = new QHBoxLayout;
    previewHead->addWidget(makeSectionTitle(panel, tr("Preview")), 1);
    previewHead->addWidget(makeHelpButton(panel, tr("A sample of GTK3 widgets drawn with the selected theme by a "
                                                    "separate helper program. It refreshes when you pick another "
                                                    "theme or when the theme's colour files change, for example "
                                                    "when Noctalia rewrites them after a wallpaper change.")));
    pl->addLayout(previewHead);
    m_preview = new QLabel(panel);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(520, 360);
    pl->addWidget(m_preview, 1);
    m_status = new QLabel(panel);
    m_status->setWordWrap(true);
    pl->addWidget(m_status);
    rl->addWidget(panel, 1);

    QVBoxLayout *cl = nullptr;
    QFrame *schemePanel = makePanel(right, &cl);
    cl->addWidget(makeSectionTitle(schemePanel, tr("Colour scheme preference")));
    m_scheme = new QComboBox(schemePanel);
    m_scheme->addItem(tr("No preference"), QStringLiteral("default"));
    m_scheme->addItem(tr("Prefer dark"), QStringLiteral("prefer-dark"));
    m_scheme->addItem(tr("Prefer light"), QStringLiteral("prefer-light"));
    cl->addWidget(withHelp(m_scheme, tr("Tells apps whether you prefer a dark or a light look. Apps that follow the "
                                        "preference (libadwaita apps, and themes with a dark variant) switch "
                                        "accordingly; other apps ignore it.")));
    cl->addWidget(makeDescLabel(schemePanel, tr("Sets org.gnome.desktop.interface color-scheme and "
                                                "gtk-application-prefer-dark-theme. libadwaita apps follow it; "
                                                "themes without a dark variant ignore it.")));
    rl->addWidget(schemePanel);

    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);
    root->addWidget(split, 1);

    m_client = new GtkPreviewClient(this);
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(200);
    connect(m_debounce, &QTimer::timeout, this, &GtkThemePage::requestPreview);
    connect(m_client, &GtkPreviewClient::rendered, this, [this](const QImage &img) {
        m_preview->setPixmap(QPixmap::fromImage(img));
        m_status->clear();
    });
    connect(m_client, &GtkPreviewClient::failed, this, [this](const QString &msg) {
        m_preview->clear();
        m_status->setText(tr("GTK preview unavailable: %1").arg(msg));
    });

    m_bind.combo(m_scheme, &ThemeState::colorScheme);
    connect(m_scheme, qOverload<int>(&QComboBox::activated), this, [this]() { m_debounce->start(); });

    connect(m_search, &QLineEdit::textChanged, this, &GtkThemePage::filter);
    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *cur) {
        if (!cur)
            return;
        m_model->state.gtkTheme = cur->data(Qt::UserRole).toString();
        m_model->touch();
        m_debounce->start();
    });
    connect(m_model, &AppModel::reloaded, this, [this]() {
        load();
        refreshPreview();
    });

    // GTK caches themes and the user gtk.css for the life of a process, and Noctalia rewrites
    // noctalia.css (imported by gtk.css) on every wallpaper change, so a change here restarts
    // the helper. The timer coalesces the several writes of one change.
    m_cssWatcher = new QFileSystemWatcher(this);
    m_cssTimer = new QTimer(this);
    m_cssTimer->setSingleShot(true);
    m_cssTimer->setInterval(400);
    connect(m_cssWatcher, &QFileSystemWatcher::fileChanged, m_cssTimer, qOverload<>(&QTimer::start));
    connect(m_cssWatcher, &QFileSystemWatcher::directoryChanged, m_cssTimer, qOverload<>(&QTimer::start));
    connect(m_cssTimer, &QTimer::timeout, this, [this]() {
        watchUserCss();
        refreshPreview();
    });
    watchUserCss();

    load();
}

// The directory catches a file that is replaced by a rename, which drops the file watch.
void GtkThemePage::watchUserCss()
{
    const QStringList old = m_cssWatcher->files() + m_cssWatcher->directories();
    if (!old.isEmpty())
        m_cssWatcher->removePaths(old);

    const QString dir = KwePaths::realConfigHome() + QStringLiteral("/gtk-3.0");
    if (!QFileInfo(dir).isDir())
        return;
    m_cssWatcher->addPath(dir);
    for (const char *name : {"gtk.css", "noctalia.css", "settings.ini"}) {
        const QString file = dir + QLatin1Char('/') + QLatin1String(name);
        if (QFileInfo::exists(file))
            m_cssWatcher->addPath(file);
    }
}

void GtkThemePage::refreshPreview()
{
    m_client->restart();
    m_previewStale = true;
    if (isVisible())
        m_debounce->start();
}

void GtkThemePage::load()
{
    populate();
    m_bind.refresh();
}

void GtkThemePage::showEvent(QShowEvent *e)
{
    PageBase::showEvent(e);
    // Render lazily: the helper is only started once the page is actually opened.
    if (m_previewStale || (m_preview->pixmap().isNull() && m_status->text().isEmpty()))
        m_debounce->start();
}

void GtkThemePage::populate()
{
    const QSignalBlocker block(m_list);
    m_list->clear();
    QListWidgetItem *current = nullptr;
    for (const ThemeScan::GtkTheme &t : std::as_const(m_model->gtkThemes)) {
        auto *it = new QListWidgetItem(t.name, m_list);
        it->setData(Qt::UserRole, t.name);
        it->setToolTip(t.path);
        if (t.name == m_model->state.gtkTheme)
            current = it;
    }
    if (current)
        m_list->setCurrentItem(current);
    filter(m_search->text());
}

void GtkThemePage::filter(const QString &text)
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        it->setHidden(!text.isEmpty() && !it->text().contains(text, Qt::CaseInsensitive));
    }
    if (m_list->currentItem())
        m_list->scrollToItem(m_list->currentItem());
}

void GtkThemePage::requestPreview()
{
    const ThemeState &s = m_model->state;
    if (s.gtkTheme.isEmpty())
        return;
    m_previewStale = false;
    m_status->setText(tr("Rendering…"));
    m_client->render(s.gtkTheme, s.colorScheme == QLatin1String("prefer-dark"),
                     s.fontName.isEmpty() ? QStringLiteral("Sans 10") : s.fontName, QSize(520, 360));
}
