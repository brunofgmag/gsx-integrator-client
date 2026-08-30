#include "TolissA340UplinkRule.h"

#include "../TolissA340.h"

namespace
{
    constexpr auto kRuleName = "toliss-a340-uplink";
}

TolissA340UplinkRule::TolissA340UplinkRule(TolissA340& aircraft) : aircraft_(&aircraft)
{
}

const char* TolissA340UplinkRule::Name() const
{
    return kRuleName;
}

RuleVerdict TolissA340UplinkRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TolissA340UplinkRule::Act(const RuleContext&, VariableWriter&)
{
    if (!aircraft_->IsPowered())
    {
        return;
    }

    aircraft_->AdvanceUplink();
}
