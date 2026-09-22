#pragma once
#include <QFutureWatcher>
#include <QMainWindow>

#include "appmodel.h"
#include "core/applypipeline.h"
#include "core/qtct.h"

class PageBase;
class QCloseEvent;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QShowEvent;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QTextEdit;

// Sidebar + stacked pages + footer (pending indicator, Apply, Close) + result log.
// The sidebar width and the log height are both draggable and remembered.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    struct QtSection {
        QListWidgetItem *header = nullptr;
        QList<QListWidgetItem *> items;
        QList<PageBase *> pages;
    };

    void buildUi();
    void buildPages();
    QListWidgetItem *addHeader(const QString &title);
    QListWidgetItem *addPage(PageBase *page, const QString &title);
    QtSection addQtSection(QtTarget target, const QString &headerTitle);

    void updateLinkState();
    void refreshPending();
    int pendingCount() const;
    QString pendingHtml();   // what Apply would change, for the bottom pane
    ApplyPlan buildPlan();
    void startApply();
    void applyFinished();
    void exportNow();
    void reloadAll();
    void appendLog(const QString &html);
    QList<PageBase *> activeQtPages() const;

    AppModel m_model;

    QSplitter *m_navSplit = nullptr;   // sidebar | pages
    QSplitter *m_logSplit = nullptr;   // (sidebar | pages) over the result log
    bool m_hasSavedLayout = false;
    QListWidget *m_nav = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLabel *m_pending = nullptr;
    QPushButton *m_apply = nullptr;
    QPushButton *m_close = nullptr;
    QTabWidget *m_tabs = nullptr;        // bottom pane: unapplied changes | results
    QTextEdit *m_pendingView = nullptr;
    QPlainTextEdit *m_log = nullptr;
    int m_lastPending = 0;

    QList<PageBase *> m_allPages;
    QListWidgetItem *m_overviewItem = nullptr;
    QtSection m_qt6;
    QtSection m_qt5;

    QFutureWatcher<ApplyReport> m_watcher;
    bool m_running = false;
    bool m_commitOnSuccess = true;   // false while running "Export files now"
};
