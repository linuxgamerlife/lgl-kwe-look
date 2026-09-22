#pragma once
#include <QLabel>
#include <QVariantMap>

#include "../pagebase.h"
#include "core/qtct.h"
#include "core/sessionsinks.h"

// Base of the five qt5ct/qt6ct pages. Each page keeps its own draft (the widgets),
// compares a snapshot of it with the state it was loaded in, and writes only its
// own keys on Apply.
class QtPageBase : public PageBase
{
    Q_OBJECT
public:
    QtPageBase(QtTarget target, QWidget *parent) : PageBase(parent), m_target(target) {}

    QtTarget target() const { return m_target; }

    // While linked, the Qt6 pages write qt5ct.conf and qt6ct.conf together, so the header says so.
    // The values shown are still read from this page's own file.
    void showLinked(bool linked)
    {
        if (!layout() || !layout()->itemAt(0) || !layout()->itemAt(0)->widget())
            return;
        const QList<QLabel *> labels =
            layout()->itemAt(0)->widget()->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
        if (labels.isEmpty())
            return;
        if (m_headerTitle.isNull()) {
            m_headerTitle = labels.at(0)->text();
            m_headerSubtitle = labels.size() > 1 ? labels.at(1)->text() : QString();
        }

        const bool both = linked && m_target == QtTarget::Qt6;
        const auto widen = [this](QString text) {
            return text.replace(QtCt::majorName(m_target), QStringLiteral("Qt5 + Qt6"))
                .replace(QtCt::toolName(m_target), QStringLiteral("qt5ct + qt6ct"));
        };
        labels.at(0)->setText(both ? widen(m_headerTitle) : m_headerTitle);
        if (labels.size() > 1) {
            labels.at(1)->setText(both ? widen(m_headerSubtitle) + QLatin1Char(' ')
                                             + tr("Apply writes qt5ct.conf and qt6ct.conf; the values shown are "
                                                  "read from qt6ct.conf.")
                                       : m_headerSubtitle);
        }
    }

    bool isDirty() const override { return snapshot() != m_loaded; }
    void markClean() override { m_loaded = snapshot(); }
    void setForceWrite(bool force) override { m_force = force; }

    void addToPlan(ApplyPlan &plan, bool linked) override
    {
        if (!isDirty() && !m_force)
            return;
        // Linked: qt5ct.conf and qt6ct.conf together, each with values in its own format.
        const QList<QtTarget> targets = linked ? QList<QtTarget>{QtTarget::Qt5, QtTarget::Qt6}
                                               : QList<QtTarget>{m_target};
        for (QtTarget t : targets)
            Sinks::addQtCtSteps(plan, t, false, updatesFor(t), pageName());
    }

protected:
    virtual QVariantMap snapshot() const = 0;
    virtual QList<IniFile::Update> updatesFor(QtTarget target) const = 0;
    virtual QString pageName() const = 0;

    // Call once the widgets reflect the file, and after every user edit.
    void loaded() { m_loaded = snapshot(); emit dirtyChanged(); }
    void edited() { emit dirtyChanged(); }

    QtTarget m_target;
    bool m_force = false;

private:
    QVariantMap m_loaded;
    QString m_headerTitle;      // as built, before showLinked() widened it
    QString m_headerSubtitle;
};
