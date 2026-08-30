#include "FenixA32xEfbSetupRule.h"

#include "../FenixA32x.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-efb-setup";
}

FenixA32xEfbSetupRule::FenixA32xEfbSetupRule(FenixA32x& aircraft) : aircraft_(&aircraft)
{
}

const char* FenixA32xEfbSetupRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xEfbSetupRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xEfbSetupRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->EnsureEfbInitialized();
}
