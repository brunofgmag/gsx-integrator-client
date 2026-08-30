#include "PmdgPayloadRule.h"

#include "../PmdgAircraft.h"

namespace
{
    constexpr auto kRuleName = "pmdg-payload";
}

PmdgPayloadRule::PmdgPayloadRule(PmdgAircraft& aircraft) : aircraft_(&aircraft)
{
}

const char* PmdgPayloadRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgPayloadRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgPayloadRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->TrimPayload();
}
