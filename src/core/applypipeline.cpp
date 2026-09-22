#include "applypipeline.h"

#include <algorithm>

void ApplyPlan::add(int order, const QString &label, std::function<StepResult()> run)
{
    m_steps.append({order, label, std::move(run)});
}

QList<ApplyStep> ApplyPlan::sorted() const
{
    QList<ApplyStep> out = m_steps;
    std::stable_sort(out.begin(), out.end(),
                     [](const ApplyStep &a, const ApplyStep &b) { return a.order < b.order; });
    return out;
}

bool ApplyReport::hasFailures() const
{
    return std::any_of(lines.cbegin(), lines.cend(), [](const ApplyLine &l) {
        return l.result.status == StepResult::Failed;
    });
}

bool ApplyReport::hasWarnings() const
{
    return std::any_of(lines.cbegin(), lines.cend(), [](const ApplyLine &l) {
        return l.result.status == StepResult::Warning;
    });
}

namespace ApplyPipeline {

ApplyReport run(const ApplyPlan &plan)
{
    ApplyReport report;
    const QList<ApplyStep> steps = plan.sorted();
    for (const ApplyStep &s : steps) {
        StepResult r = s.run ? s.run() : StepResult::fail(QStringLiteral("no action"));
        report.lines.append({s.label, r});
    }
    return report;
}

}  // namespace ApplyPipeline
