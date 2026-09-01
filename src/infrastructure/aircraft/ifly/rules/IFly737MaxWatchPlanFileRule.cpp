#include "IFly737MaxWatchPlanFileRule.h"

#include <utility>

#include "../../../ifly/IFlyPlanFile.h"
#include "../../../../domain/model/AutomationStatus.h"

namespace
{
    constexpr auto kRuleName = "ifly-737max-watch-plan-file";
}

IFly737MaxWatchPlanFileRule::IFly737MaxWatchPlanFileRule(IFlyPlanImport& planImport,
                                                         const AutomationStatus& status,
                                                         std::optional<std::filesystem::path> appDataRoot)
    : planImport_(&planImport), status_(&status), appDataRoot_(std::move(appDataRoot))
{
}

const char* IFly737MaxWatchPlanFileRule::Name() const
{
    return kRuleName;
}

RuleCadence IFly737MaxWatchPlanFileRule::Cadence() const
{
    return RuleCadence::Slow;
}

RuleVerdict IFly737MaxWatchPlanFileRule::Evaluate(const RuleContext&)
{
    planImport_->Observe(PlanDirectory(), status_->planGeneratedEpoch);

    return RuleVerdict::Pass();
}

void IFly737MaxWatchPlanFileRule::Act(const RuleContext&, VariableWriter&)
{
}

std::optional<std::filesystem::path> IFly737MaxWatchPlanFileRule::PlanDirectory() const
{
    return appDataRoot_.has_value() ? IFlyPlanFile::DirectoryUnder(*appDataRoot_)
                                    : IFlyPlanFile::DirectoryFor();
}
