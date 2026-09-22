#pragma once
#include <QHash>
#include <QIcon>

#include "pagebase.h"

class AppModel;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;

// Icon theme: shared by GTK, Qt5 and Qt6, so a single list drives every sink.
class IconsPage : public PageBase
{
    Q_OBJECT
public:
    explicit IconsPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;
    bool wantsScrollArea() const override { return false; }

private:
    void populate();
    void filter(const QString &text);
    void updatePreview();
    void fitList();
    QString resolveIcon(const QString &folder, const QString &name, int size, int depth = 0) const;
    QPixmap iconPixmap(const QString &folder, const QString &name, int size) const;
    QIcon glyphStrip(const QString &folder);

    AppModel *m_model;
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    QWidget *m_left = nullptr;
    QGridLayout *m_grid = nullptr;
    QHBoxLayout *m_glyphRow = nullptr;
    QHash<QString, QIcon> m_glyphCache;   // theme folder -> the glyphs shown beside its name
    QLabel *m_title = nullptr;
    QLabel *m_note = nullptr;
};
