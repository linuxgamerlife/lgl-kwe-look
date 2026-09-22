#pragma once
#include <QVBoxLayout>
#include <QWidget>

#include "core/applypipeline.h"
#include "pagehelpers.h"

// Base class of every page. Pages edit either the shared AppModel (icons, cursors,
// GTK) or their own draft (the Qt pages), and contribute steps to the single Apply.
class PageBase : public QWidget
{
    Q_OBJECT
public:
    explicit PageBase(QWidget *parent = nullptr) : QWidget(parent) {}

    // (Re)read from disk / the model. Discards unsaved edits of this page.
    virtual void load() {}
    // Unsaved edits of pages that keep their own draft.
    virtual bool isDirty() const { return false; }
    // Adds this page's writes to the plan. `linked`: write qt5ct.conf and qt6ct.conf.
    virtual void addToPlan(ApplyPlan &plan, bool linked) { Q_UNUSED(plan) Q_UNUSED(linked) }
    // Called after a successful Apply so the page can reset its dirty baseline.
    virtual void markClean() {}
    // "Re-apply all": write this page's keys even when nothing changed.
    virtual void setForceWrite(bool force) { Q_UNUSED(force) }
    // Pages with their own scrolling lists return false and are not wrapped in a scroll area.
    virtual bool wantsScrollArea() const { return true; }

signals:
    void dirtyChanged();

protected:
    // Creates the root layout with the page header already in place.
    QVBoxLayout *initPage(const QString &title, const QString &subtitle)
    {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(16, 12, 16, 12);
        root->setSpacing(12);
        root->addWidget(makePageHeader(this, title, subtitle));
        return root;
    }
};
