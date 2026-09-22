#pragma once
#include "../pagebase.h"
#include "gtkbindings.h"

class GtkPreviewClient;
class QComboBox;
class QFileSystemWatcher;
class QLabel;
class QLineEdit;
class QListWidget;
class QTimer;

// GTK widget theme, colour-scheme preference and a live preview rendered by the
// separate GTK3 helper process.
class GtkThemePage : public PageBase
{
    Q_OBJECT
public:
    explicit GtkThemePage(AppModel *model, QWidget *parent = nullptr);

    void load() override;
    bool wantsScrollArea() const override { return false; }

protected:
    void showEvent(QShowEvent *e) override;

private:
    void populate();
    void filter(const QString &text);
    void requestPreview();
    void watchUserCss();
    void refreshPreview();   // fresh helper, then render again

    AppModel *m_model;
    Bindings m_bind;
    GtkPreviewClient *m_client = nullptr;
    QTimer *m_debounce = nullptr;
    QFileSystemWatcher *m_cssWatcher = nullptr;   // ~/.config/gtk-3.0: gtk.css, noctalia.css, settings.ini
    QTimer *m_cssTimer = nullptr;
    bool m_previewStale = false;                  // the files changed while the page was hidden
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    QLabel *m_preview = nullptr;
    QLabel *m_status = nullptr;
    QComboBox *m_scheme = nullptr;
};
