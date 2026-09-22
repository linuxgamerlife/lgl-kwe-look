#pragma once
#include <QList>
#include <QString>
#include <functional>

#include "stepresult.h"

// Sink ordering. Lower runs first. The order mirrors what Kinetic Settings does
// for icons and cursors so the compositor config is in place before the
// KGlobalSettings notification fires.
namespace ApplyOrder {
enum : int {
    AppearanceKwe = 10,   // ~/.config/kineticwe/appearance.kwe
    CompositorKwe = 20,   // ~/.config/kineticwe/kineticwe.kwe [Mouse]
    KdeGlobals    = 30,   // kdeglobals [Icons] / [KDE]
    QtCtConfig    = 40,   // qt5ct.conf / qt6ct.conf
    GSettings     = 50,   // org.gnome.desktop.*
    GtkExport     = 60,   // settings.ini, gtkrc-2.0, xsettingsd, index.theme, GTK4 links
    Flatpak       = 70,
    Notify        = 90    // KGlobalSettings::notifyChange, last
};
}

struct ApplyStep {
    int order = 0;
    QString label;
    std::function<StepResult()> run;
};

// A list of writes collected from every page, executed once by Apply.
class ApplyPlan
{
public:
    void add(int order, const QString &label, std::function<StepResult()> run);
    bool isEmpty() const { return m_steps.isEmpty(); }
    int size() const { return int(m_steps.size()); }
    // Stable-sorted by order: steps with the same order keep insertion order.
    QList<ApplyStep> sorted() const;

private:
    QList<ApplyStep> m_steps;
};

struct ApplyLine {
    QString label;
    StepResult result;
};

struct ApplyReport {
    QList<ApplyLine> lines;

    bool hasFailures() const;
    bool hasWarnings() const;
};

namespace ApplyPipeline {
// Runs every step, never stops early: one failed sink must not stop the others
// from being written. Safe to call from a worker thread.
ApplyReport run(const ApplyPlan &plan);
}
