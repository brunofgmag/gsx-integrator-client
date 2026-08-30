#include "PmdgGroundConnectionRule.h"

#include "../PmdgAircraft.h"

namespace
{
    constexpr auto kRuleName = "pmdg-ground-connection";
}

PmdgGroundConnectionRule::PmdgGroundConnectionRule(PmdgAircraft& aircraft) : aircraft_(&aircraft)
{
}

const char* PmdgGroundConnectionRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgGroundConnectionRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgGroundConnectionRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->ReconcileGroundConnection();
}
