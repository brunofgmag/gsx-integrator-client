#include "TfdiMd11EfbTargetRule.h"

#include "../TfdiMd11.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-efb-targets";
}

TfdiMd11EfbTargetRule::TfdiMd11EfbTargetRule(TfdiMd11& aircraft) : aircraft_(&aircraft)
{
}

const char* TfdiMd11EfbTargetRule::Name() const
{
    return kRuleName;
}

RuleCadence TfdiMd11EfbTargetRule::Cadence() const
{
    return RuleCadence::Slow;
}

RuleVerdict TfdiMd11EfbTargetRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11EfbTargetRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->CommitPendingEfbTargets();
}
