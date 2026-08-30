#include "IFly737MaxPlanImportRule.h"

#include "../IFly737Max.h"

namespace
{
    constexpr auto kRuleName = "ifly-737max-plan-import";
}

IFly737MaxPlanImportRule::IFly737MaxPlanImportRule(IFly737Max& aircraft) : aircraft_(&aircraft)
{
}

const char* IFly737MaxPlanImportRule::Name() const
{
    return kRuleName;
}

RuleCadence IFly737MaxPlanImportRule::Cadence() const
{
    return RuleCadence::Slow;
}

RuleVerdict IFly737MaxPlanImportRule::Evaluate(const RuleContext&)
{
    aircraft_->ImportPlan();

    return RuleVerdict::Pass();
}

void IFly737MaxPlanImportRule::Act(const RuleContext&, VariableWriter&)
{
}
