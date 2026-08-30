#include "FenixA32xRefuelSystemRule.h"

#include "../FenixA32x.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-refuel-system";
}

FenixA32xRefuelSystemRule::FenixA32xRefuelSystemRule(FenixA32x& aircraft) : aircraft_(&aircraft)
{
}

const char* FenixA32xRefuelSystemRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xRefuelSystemRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xRefuelSystemRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DisarmRefuelSystemWhenDone();
}
