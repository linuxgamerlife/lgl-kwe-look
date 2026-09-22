#pragma once
#include "pagebase.h"

class AppModel;
class QCheckBox;
class QVBoxLayout;

// Session information, component availability and the options that span pages.
class OverviewPage : public PageBase
{
    Q_OBJECT
public:
    explicit OverviewPage(AppModel *model, QWidget *parent = nullptr);

    void load() override;

signals:
    void reapplyRequested();
    void reloadRequested();

private:
    void buildComponentRows();
    void buildOutputRows();   // where an Apply writes for this session

    AppModel *m_model;
    QCheckBox *m_link = nullptr;
    QVBoxLayout *m_componentsLayout = nullptr;
    QVBoxLayout *m_outputsLayout = nullptr;
};
