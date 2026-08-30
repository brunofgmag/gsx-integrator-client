#include "TolissA340DoorRule.h"

#include "../TolissA340.h"

namespace
{
    constexpr auto kRuleName = "toliss-a340-doors";
}

TolissA340DoorRule::TolissA340DoorRule(TolissA340& aircraft) : aircraft_(&aircraft)
{
}

const char* TolissA340DoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict TolissA340DoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TolissA340DoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DriveDoors();
}
