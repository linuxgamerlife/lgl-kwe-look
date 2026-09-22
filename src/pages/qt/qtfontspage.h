#pragma once
#include <QFont>

#include "qtpagebase.h"

class AppModel;
class QLabel;

// General and fixed-width font. The strings are written in the config format of each
// target (Qt5 and Qt6 differ, see QtCt::fontToConfigString).
class QtFontsPage : public QtPageBase
{
    Q_OBJECT
public:
    QtFontsPage(QtTarget target, AppModel *model, QWidget *parent = nullptr);

    void load() override;

protected:
    QVariantMap snapshot() const override;
    QList<IniFile::Update> updatesFor(QtTarget target) const override;
    QString pageName() const override { return tr("fonts"); }

private:
    void choose(QFont *font, QLabel *label);
    static void show(const QFont &font, QLabel *label);

    AppModel *m_model;
    QFont m_general;
    QFont m_fixed;
    QLabel *m_generalLabel = nullptr;
    QLabel *m_fixedLabel = nullptr;
};
