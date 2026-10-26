#include "Bbs757DoorsFollowGsxRule.h"

#include "../Bbs757.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "bbs-757-doors-follow-gsx";

    constexpr double kDoorSwitchOpen = 1.0;
    constexpr double kDoorSwitchClosed = 0.0;
}

Bbs757DoorsFollowGsxRule::Bbs757DoorsFollowGsxRule(GsxDoorSync& doors) : doors_(&doors)
{
}

const char* Bbs757DoorsFollowGsxRule::Name() const
{
    return kRuleName;
}

RuleVerdict Bbs757DoorsFollowGsxRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Bbs757DoorsFollowGsxRule::Act(const RuleContext&, VariableWriter& writer)
{
    doors_->Sync([&writer](const GsxDoor door, const bool open)
    {
        writer.SetLVar(Bbs757::DoorSwitch(door), open ? kDoorSwitchOpen : kDoorSwitchClosed);
    });
}
