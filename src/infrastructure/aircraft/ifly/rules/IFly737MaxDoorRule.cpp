#include "IFly737MaxDoorRule.h"

#include "../IFly737Max.h"

namespace
{
    constexpr auto kRuleName = "ifly-737max-doors";
}

IFly737MaxDoorRule::IFly737MaxDoorRule(IFly737Max& aircraft) : aircraft_(&aircraft)
{
}

const char* IFly737MaxDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict IFly737MaxDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void IFly737MaxDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DriveDoors();
}
