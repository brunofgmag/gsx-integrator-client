#include "FenixA32xDoorRule.h"

#include "../FenixA32x.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-doors";
}

FenixA32xDoorRule::FenixA32xDoorRule(FenixA32x& aircraft) : aircraft_(&aircraft)
{
}

const char* FenixA32xDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DriveDoors();
}
