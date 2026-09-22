#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QSignalBlocker>
#include <functional>

#include "appmodel.h"

// Two-way binding between a widget and one ThemeState field. The widget writes
// the field and calls AppModel::touch(); refresh() pushes the model back into the
// widgets after a reload.
class Bindings
{
public:
    explicit Bindings(AppModel *model) : m_model(model) {}

    void combo(QComboBox *c, QString ThemeState::*field)
    {
        QObject::connect(c, qOverload<int>(&QComboBox::activated), c, [this, c, field](int i) {
            m_model->state.*field = c->itemData(i).toString();
            m_model->touch();
        });
        m_refresh << [this, c, field]() {
            const QSignalBlocker b(c);
            const QString v = m_model->state.*field;
            int i = c->findData(v);
            if (i < 0) {   // a value we do not list must still show, or Apply would silently reset it
                c->addItem(v, v);
                i = c->count() - 1;
            }
            c->setCurrentIndex(i);
        };
    }

    void check(QCheckBox *c, bool ThemeState::*field)
    {
        QObject::connect(c, &QCheckBox::toggled, c, [this, field](bool on) {
            m_model->state.*field = on;
            m_model->touch();
        });
        m_refresh << [this, c, field]() {
            const QSignalBlocker b(c);
            c->setChecked(m_model->state.*field);
        };
    }

    void checkPref(QCheckBox *c, bool Preferences::*field)
    {
        QObject::connect(c, &QCheckBox::toggled, c, [this, field](bool on) {
            m_model->prefs.*field = on;
            m_model->touch();
        });
        m_refresh << [this, c, field]() {
            const QSignalBlocker b(c);
            c->setChecked(m_model->prefs.*field);
        };
    }

    void refresh()
    {
        for (const auto &f : m_refresh)
            f();
    }

private:
    AppModel *m_model;
    QList<std::function<void()>> m_refresh;
};
