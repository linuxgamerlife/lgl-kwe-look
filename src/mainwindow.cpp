#include "mainwindow.h"

#include "core/kwepaths.h"
#include "core/migration.h"
#include "pagehelpers.h"
#include "pages/cursorspage.h"
#include "pages/gtk/gtkexportpage.h"
#include "pages/gtk/gtkfontspage.h"
#include "pages/gtk/gtkotherpage.h"
#include "pages/gtk/gtkthemepage.h"
#include "pages/iconspage.h"
#include "pages/overviewpage.h"
#include "pages/qt/qtappearancepage.h"
#include "pages/qt/qtfontspage.h"
#include "pages/qt/qtinterfacepage.h"
#include "pages/qt/qtstylesheetspage.h"
#include "pages/qt/qttroubleshootingpage.h"

#include <QCloseEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizeGrip>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
constexpr int kStackIndexRole = Qt::UserRole;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("LGL KWE Look"));
    setMinimumSize(1000, 700);   // the benchmark's 1100x1000 does not fit small screens

    m_model.load();
    buildUi();
    buildPages();
    updateLinkState();
    refreshPending();

    // Own window state lives in ~/.config/lgl-kwe-look, never in qt6ct.conf: a write there
    // makes the platform plugin reload every running Qt app.
    const QByteArray geometry = Prefs::loadWindowGeometry();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    connect(&m_watcher, &QFutureWatcher<ApplyReport>::finished, this, &MainWindow::applyFinished);
    connect(&m_model, &AppModel::changed, this, &MainWindow::refreshPending);
    connect(&m_model, &AppModel::linkChanged, this, [this]() {
        // Qt5 edits made while unlinked would otherwise be written on top of the linked Qt6 values.
        for (PageBase *p : std::as_const(m_qt5.pages))
            p->load();
        updateLinkState();
        refreshPending();
    });
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Two draggable dividers: sidebar | pages, and (both) over the result log. A widget with a
    // fixed size inside a splitter cannot be moved, so only limits are set here.
    m_logSplit = new GripSplitter(Qt::Vertical, central);
    m_navSplit = new GripSplitter(Qt::Horizontal, m_logSplit);
    m_nav = new QListWidget(m_navSplit);
    m_nav->setMinimumWidth(150);
    m_nav->setMaximumWidth(420);
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setSpacing(1);
    m_stack = new QStackedWidget(m_navSplit);
    m_navSplit->addWidget(m_nav);
    m_navSplit->addWidget(m_stack);
    m_navSplit->setStretchFactor(0, 0);
    m_navSplit->setStretchFactor(1, 1);

    // Bottom pane: what Apply is about to change, and what it did.
    m_tabs = new QTabWidget(m_logSplit);
    m_tabs->setDocumentMode(true);
    m_tabs->setMinimumHeight(90);
    m_pendingView = new QTextEdit(m_tabs);
    m_pendingView->setReadOnly(true);
    m_log = new QPlainTextEdit(m_tabs);
    m_log->setReadOnly(true);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_log->setMaximumBlockCount(500);
    m_log->setPlaceholderText(tr("Results of Apply appear here."));
    m_tabs->addTab(m_pendingView, tr("Unapplied changes"));
    m_tabs->addTab(m_log, tr("Results"));
    m_logSplit->addWidget(m_navSplit);
    m_logSplit->addWidget(m_tabs);
    m_logSplit->setStretchFactor(0, 1);
    m_logSplit->setStretchFactor(1, 0);
    outer->addWidget(m_logSplit, 1);

    // Saved positions win; otherwise showEvent() sets the first-run sizes from the real window size.
    const QByteArray navState = Prefs::loadLayoutState(QStringLiteral("nav_split"));
    const QByteArray logState = Prefs::loadLayoutState(QStringLiteral("log_split"));
    m_hasSavedLayout = !navState.isEmpty() && !logState.isEmpty();
    if (m_hasSavedLayout) {
        m_navSplit->restoreState(navState);
        m_logSplit->restoreState(logState);
    }

    auto *footer = new QWidget(central);
    auto *fl = new QHBoxLayout(footer);
    fl->setContentsMargins(12, 8, 12, 8);
    m_pending = new QLabel(footer);
    m_apply = new QPushButton(tr("Apply"), footer);
    m_apply->setDefault(true);
    m_close = new QPushButton(tr("Close"), footer);
    fl->addWidget(m_pending, 1);
    fl->addWidget(m_apply);
    fl->addWidget(m_close);
    // Grab bar: a window without frame decorations (a tiling session) can still be resized.
    fl->addWidget(new QSizeGrip(footer), 0, Qt::AlignRight | Qt::AlignBottom);
    outer->addWidget(footer);

    setCentralWidget(central);

    connect(m_nav, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *cur) {
        if (cur && cur->data(kStackIndexRole).isValid())
            m_stack->setCurrentIndex(cur->data(kStackIndexRole).toInt());
    });
    connect(m_apply, &QPushButton::clicked, this, &MainWindow::startApply);
    connect(m_close, &QPushButton::clicked, this, &QWidget::close);
}

QListWidgetItem *MainWindow::addHeader(const QString &title)
{
    auto *it = new QListWidgetItem(title.toUpper(), m_nav);
    QFont f = it->font();
    f.setBold(true);
    f.setPointSize(qMax(7, f.pointSize() - 2));
    it->setFont(f);
    it->setFlags(Qt::NoItemFlags);   // not selectable
    QColor c = palette().color(QPalette::WindowText);
    c.setAlphaF(0.55f);
    it->setForeground(c);
    return it;
}

QListWidgetItem *MainWindow::addPage(PageBase *page, const QString &title)
{
    QWidget *host = page;
    if (page->wantsScrollArea()) {
        auto *area = new SmoothScrollArea;
        area->setWidget(page);
        host = area;
    }
    const int index = m_stack->addWidget(host);
    auto *it = new QListWidgetItem(QStringLiteral("  ") + title, m_nav);
    it->setData(kStackIndexRole, index);
    m_allPages << page;
    connect(page, &PageBase::dirtyChanged, this, &MainWindow::refreshPending);
    return it;
}

MainWindow::QtSection MainWindow::addQtSection(QtTarget target, const QString &headerTitle)
{
    QtSection s;
    s.header = addHeader(headerTitle);

    const auto add = [&](PageBase *page, const QString &title) {
        s.items << addPage(page, title);
        s.pages << page;
    };
    add(new QtAppearancePage(target, &m_model), tr("Appearance"));
    add(new QtFontsPage(target, &m_model), tr("Fonts"));
    add(new QtInterfacePage(target, &m_model), tr("Interface"));
    add(new QtStyleSheetsPage(target, &m_model), tr("Style Sheets"));
    add(new QtTroubleshootingPage(target, &m_model), tr("Troubleshooting"));
    return s;
}

void MainWindow::buildPages()
{
    auto *overview = new OverviewPage(&m_model);
    m_overviewItem = addPage(overview, tr("Overview"));
    connect(overview, &OverviewPage::reapplyRequested, this, [this]() {
        m_model.forceReapply = true;
        startApply();
    });
    connect(overview, &OverviewPage::reloadRequested, this, &MainWindow::reloadAll);

    addHeader(tr("Shared"));
    addPage(new IconsPage(&m_model), tr("Icons"));
    addPage(new CursorsPage(&m_model), tr("Cursors"));

    addHeader(tr("GTK"));
    addPage(new GtkThemePage(&m_model), tr("Theme"));
    addPage(new GtkFontsPage(&m_model), tr("Fonts"));
    addPage(new GtkOtherPage(&m_model), tr("Other"));
    auto *exportPage = new GtkExportPage(&m_model);
    addPage(exportPage, tr("Export and Flatpak"));
    connect(exportPage, &GtkExportPage::exportNowRequested, this, &MainWindow::exportNow);

    m_qt6 = addQtSection(QtTarget::Qt6, tr("Qt6"));
    m_qt5 = addQtSection(QtTarget::Qt5, tr("Qt5"));

    m_nav->setCurrentItem(m_overviewItem);
}

void MainWindow::updateLinkState()
{
    const bool linked = m_model.prefs.linkQt;
    m_qt6.header->setText((linked ? tr("Qt5 + Qt6") : tr("Qt6")).toUpper());
    m_qt5.header->setHidden(linked);
    for (QListWidgetItem *it : std::as_const(m_qt5.items))
        it->setHidden(linked);
    for (PageBase *p : std::as_const(m_qt6.pages)) {
        if (auto *qp = qobject_cast<QtPageBase *>(p))
            qp->showLinked(linked);
    }

    // Do not leave the user on a page that just disappeared.
    if (linked && m_nav->currentItem() && m_nav->currentItem()->isHidden())
        m_nav->setCurrentItem(m_overviewItem);
}

QList<PageBase *> MainWindow::activeQtPages() const
{
    // Linked: the Qt6 pages write both files. Unlinked: each set writes its own.
    QList<PageBase *> pages = m_qt6.pages;
    if (!m_model.prefs.linkQt)
        pages << m_qt5.pages;
    return pages;
}

int MainWindow::pendingCount() const
{
    int n = 0;
    if (m_model.isDirty())
        ++n;
    for (PageBase *p : activeQtPages()) {
        if (p->isDirty())
            ++n;
    }
    return n;
}

void MainWindow::refreshPending()
{
    const int n = pendingCount();
    if (n > 0) {
        m_pending->setText(QStringLiteral("<span style='color:#cc7700;'>●</span> ")
                           + tr("Unapplied changes in %n area(s)", "", n));
    } else {
        m_pending->setText(tr("No unapplied changes"));
    }
    m_apply->setEnabled(!m_running);

    m_pendingView->setHtml(pendingHtml());
    m_tabs->setTabText(0, n > 0 ? tr("Unapplied changes (%1)").arg(n) : tr("Unapplied changes"));
    // New edits after an Apply: bring the list back to the front.
    if (n > 0 && m_lastPending == 0)
        m_tabs->setCurrentWidget(m_pendingView);
    m_lastPending = n;
}

QString MainWindow::pendingHtml()
{
    const ThemeState &s = m_model.state;
    const ThemeState &b = m_model.baseline;
    QStringList lines;

    const auto change = [&lines](const QString &what, const QString &from, const QString &to) {
        lines << QStringLiteral("<b>%1</b>: %2 → %3").arg(what.toHtmlEscaped(), from.toHtmlEscaped(), to.toHtmlEscaped());
    };
    const auto onOff = [this](bool v) { return v ? tr("on") : tr("off"); };
    const auto num = [](double v) { return QString::number(v, 'f', 2); };

    if (s.iconTheme != b.iconTheme)               change(tr("Icon theme"), b.iconTheme, s.iconTheme);
    if (s.cursorTheme != b.cursorTheme)           change(tr("Cursor theme"), b.cursorTheme, s.cursorTheme);
    if (s.cursorSize != b.cursorSize)             change(tr("Cursor size"), QString::number(b.cursorSize), QString::number(s.cursorSize));
    if (s.gtkTheme != b.gtkTheme)                 change(tr("GTK theme"), b.gtkTheme, s.gtkTheme);
    if (s.colorScheme != b.colorScheme)           change(tr("GTK colour scheme"), b.colorScheme, s.colorScheme);
    if (s.fontName != b.fontName)                 change(tr("GTK font"), b.fontName, s.fontName);
    if (qAbs(s.textScalingFactor - b.textScalingFactor) > 1e-6)
        change(tr("Text scaling factor"), num(b.textScalingFactor), num(s.textScalingFactor));
    if (s.fontAntialiasing != b.fontAntialiasing) change(tr("Antialiasing"), b.fontAntialiasing, s.fontAntialiasing);
    if (s.fontHinting != b.fontHinting)           change(tr("Hinting"), b.fontHinting, s.fontHinting);
    if (s.fontRgbaOrder != b.fontRgbaOrder)       change(tr("Subpixel order"), b.fontRgbaOrder, s.fontRgbaOrder);
    if (s.toolbarStyle != b.toolbarStyle)         change(tr("Toolbar style"), b.toolbarStyle, s.toolbarStyle);
    if (s.toolbarIconsSize != b.toolbarIconsSize) change(tr("Toolbar icon size"), b.toolbarIconsSize, s.toolbarIconsSize);
    if (s.eventSounds != b.eventSounds)           change(tr("Event sounds"), onOff(b.eventSounds), onOff(s.eventSounds));
    if (s.inputFeedbackSounds != b.inputFeedbackSounds)
        change(tr("Input feedback sounds"), onOff(b.inputFeedbackSounds), onOff(s.inputFeedbackSounds));
    if (s.iniToolbarStyle != b.iniToolbarStyle)   change(tr("settings.ini toolbar style"), b.iniToolbarStyle, s.iniToolbarStyle);
    if (s.iniToolbarIconSize != b.iniToolbarIconSize)
        change(tr("settings.ini toolbar icon size"), b.iniToolbarIconSize, s.iniToolbarIconSize);
    if (s.buttonImages != b.buttonImages)         change(tr("Images on buttons"), onOff(b.buttonImages), onOff(s.buttonImages));
    if (s.menuImages != b.menuImages)             change(tr("Images in menus"), onOff(b.menuImages), onOff(s.menuImages));

    if (m_model.prefs.linkQt != m_model.prefsBaseline.linkQt)
        change(tr("Link Qt5 and Qt6"), onOff(m_model.prefsBaseline.linkQt), onOff(m_model.prefs.linkQt));
    Preferences otherPrefs = m_model.prefs;
    otherPrefs.linkQt = m_model.prefsBaseline.linkQt;
    if (otherPrefs != m_model.prefsBaseline)
        lines << tr("<b>Export and Flatpak options</b> changed");

    // The Qt pages keep their own drafts: name the pages that were edited.
    const auto dirtyPages = [&lines](const QtSection &section, const QString &group) {
        for (int i = 0; i < section.pages.size(); ++i) {
            if (section.pages.at(i)->isDirty())
                lines << QStringLiteral("<b>%1</b> › %2").arg(group.toHtmlEscaped(),
                                                                 section.items.at(i)->text().trimmed().toHtmlEscaped());
        }
    };
    dirtyPages(m_qt6, m_model.prefs.linkQt ? tr("Qt5 + Qt6") : tr("Qt6"));
    if (!m_model.prefs.linkQt)
        dirtyPages(m_qt5, tr("Qt5"));

    if (m_model.forceReapply)
        lines << tr("<b>Re-apply all settings</b>: every file and setting is written again");

    if (lines.isEmpty())
        return QStringLiteral("<i>%1</i>").arg(tr("No unapplied changes."));

    QString html = QStringLiteral("<ul style='margin-top:0px; margin-bottom:0px;'>");
    for (const QString &line : std::as_const(lines))
        html += QStringLiteral("<li>%1</li>").arg(line);
    html += QStringLiteral("</ul><p style='margin-top:6px;'><i>%1</i></p>")
                .arg(tr("Apply will run %n step(s).", "", buildPlan().size()));
    return html;
}

ApplyPlan MainWindow::buildPlan()
{
    ApplyPlan plan;
    const bool force = m_model.forceReapply;

    Sinks::addSessionSteps(plan, m_model.state, m_model.baseline, force);

    Sinks::GtkContext ctx;
    ctx.state = m_model.state;
    ctx.baseline = m_model.baseline;
    ctx.prefs = m_model.prefs;
    ctx.prefsBaseline = m_model.prefsBaseline;
    ctx.preservedIniLines = m_model.preservedIniLines;
    ctx.gtkThemePath = m_model.gtkThemePath(m_model.state.gtkTheme);
    ctx.force = force;
    Sinks::addGtkSteps(plan, ctx);

    for (PageBase *p : activeQtPages()) {
        p->setForceWrite(force);
        p->addToPlan(plan, m_model.prefs.linkQt);
    }
    return plan;
}

void MainWindow::startApply()
{
    if (m_running)
        return;

    const ApplyPlan plan = buildPlan();
    if (plan.isEmpty()) {
        appendLog(tr("Nothing to apply."));
        m_model.forceReapply = false;
        return;
    }

    m_running = true;
    m_commitOnSuccess = true;
    m_apply->setEnabled(false);
    appendLog(tr("<b>Applying %n step(s)…</b>", "", plan.size()));
    // The steps only touch files, gsettings and D-Bus, never widgets, so they run off the GUI thread.
    m_watcher.setFuture(QtConcurrent::run([plan]() { return ApplyPipeline::run(plan); }));
}

void MainWindow::applyFinished()
{
    const ApplyReport report = m_watcher.result();
    m_running = false;

    for (const ApplyLine &l : report.lines) {
        QString tag;
        switch (l.result.status) {
        case StepResult::Ok:      tag = QStringLiteral("<span style='color:#3db03d;'>[ok]</span>"); break;
        case StepResult::Warning: tag = QStringLiteral("<span style='color:#cc7700;'>[warn]</span>"); break;
        case StepResult::Failed:  tag = QStringLiteral("<span style='color:#cc0000;'>[FAIL]</span>"); break;
        }
        QString text = l.label.toHtmlEscaped();
        if (!l.result.detail.isEmpty())
            text += QStringLiteral(": ") + l.result.detail.toHtmlEscaped();
        appendLog(tag + QLatin1Char(' ') + text);
    }

    if (!m_commitOnSuccess) {
        // An export writes files only: pending edits stay pending.
        m_commitOnSuccess = true;
        appendLog(report.hasFailures() ? tr("<b>Export finished with errors.</b>") : tr("<b>Export finished.</b>"));
        refreshPending();
        return;
    }

    if (report.hasFailures()) {
        // Keep the edits so Apply can be retried.
        appendLog(tr("<b>Some steps failed. Your changes were kept, try Apply again.</b>"));
    } else {
        m_model.commit();
        for (PageBase *p : std::as_const(m_allPages))
            p->markClean();
        appendLog(report.hasWarnings() ? tr("<b>Applied, with warnings.</b>") : tr("<b>Applied.</b>"));
    }
    m_model.forceReapply = false;
    for (PageBase *p : std::as_const(m_allPages))
        p->setForceWrite(false);
    refreshPending();
}

void MainWindow::exportNow()
{
    if (m_running)
        return;
    Sinks::GtkContext ctx;
    ctx.state = m_model.state;
    ctx.baseline = m_model.state;
    ctx.prefs = m_model.prefs;
    ctx.prefsBaseline = m_model.prefs;
    ctx.preservedIniLines = m_model.preservedIniLines;
    ctx.gtkThemePath = m_model.gtkThemePath(m_model.state.gtkTheme);
    ctx.exportOnly = true;

    ApplyPlan plan;
    Sinks::addGtkSteps(plan, ctx);
    m_running = true;
    m_commitOnSuccess = false;
    m_apply->setEnabled(false);
    appendLog(tr("<b>Exporting GTK config files…</b>"));
    m_watcher.setFuture(QtConcurrent::run([plan]() { return ApplyPipeline::run(plan); }));
}

void MainWindow::reloadAll()
{
    if (pendingCount() > 0
        && QMessageBox::question(this, tr("Reload from disk"),
                                 tr("This discards your unapplied changes. Continue?")) != QMessageBox::Yes)
        return;
    m_model.load();
    for (PageBase *p : std::as_const(m_allPages))
        p->load();
    updateLinkState();
    refreshPending();
    appendLog(tr("Reloaded from disk."));
}

void MainWindow::appendLog(const QString &html)
{
    m_tabs->setCurrentWidget(m_log);   // show the outcome of what was just applied
    m_log->appendHtml(html);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_running) {
        event->ignore();
        return;
    }
    if (pendingCount() > 0) {
        const auto answer = QMessageBox::question(this, tr("Unapplied changes"),
                                                  tr("You have unapplied changes. Close and discard them?"));
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    Prefs::saveWindowGeometry(saveGeometry());
    Prefs::saveLayoutState(QStringLiteral("nav_split"), m_navSplit->saveState());
    Prefs::saveLayoutState(QStringLiteral("log_split"), m_logSplit->saveState());
    event->accept();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // First run: a 210 px sidebar and a 150 px bottom pane, whatever size the window opens at.
    if (!m_hasSavedLayout) {
        m_hasSavedLayout = true;
        m_navSplit->setSizes({210, qMax(300, m_navSplit->width() - 210)});
        m_logSplit->setSizes({qMax(300, m_logSplit->height() - 150), 150});
    }
}
