#include "PmdgDoorRule.h"

#include "../PmdgAircraft.h"

namespace
{
    constexpr auto kRuleName = "pmdg-doors";
}

PmdgDoorRule::PmdgDoorRule(PmdgAircraft& aircraft) : aircraft_(&aircraft)
{
}

const char* PmdgDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->SyncDoors();
}
