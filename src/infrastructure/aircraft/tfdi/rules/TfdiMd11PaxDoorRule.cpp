#include "TfdiMd11PaxDoorRule.h"

#include "../TfdiMd11.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-pax-doors";
}

TfdiMd11PaxDoorRule::TfdiMd11PaxDoorRule(TfdiMd11& aircraft) : aircraft_(&aircraft)
{
}

const char* TfdiMd11PaxDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict TfdiMd11PaxDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11PaxDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DrivePaxDoors();
}
