#pragma once
#include "qtpagebase.h"

class AppModel;
class QCheckBox;
class QListWidget;

// Applications the platform theme must leave alone, and the raster-widgets workaround.
class QtTroubleshootingPage : public QtPageBase
{
    Q_OBJECT
public:
    QtTroubleshootingPage(QtTarget target, AppModel *model, QWidget *parent = nullptr);

    void load() override;

protected:
    QVariantMap snapshot() const override;
    QList<IniFile::Update> updatesFor(QtTarget target) const override;
    QString pageName() const override { return tr("troubleshooting"); }

private:
    QStringList ignored() const;

    AppModel *m_model;
    QListWidget *m_apps = nullptr;
    QCheckBox *m_raster = nullptr;
};
