#include "TolissA340DoorsFollowGsxRule.h"

#include "../TolissA340.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "toliss-a340-doors-follow-gsx";

    constexpr double kDoorOpen = 2.0;
    constexpr double kDoorClosed = 0.0;
}

TolissA340DoorsFollowGsxRule::TolissA340DoorsFollowGsxRule(const TolissA340& aircraft, GsxDoorSync& doors)
    : aircraft_(&aircraft), doors_(&doors)
{
}

const char* TolissA340DoorsFollowGsxRule::Name() const
{
    return kRuleName;
}

RuleVerdict TolissA340DoorsFollowGsxRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TolissA340DoorsFollowGsxRule::Act(const RuleContext&, VariableWriter& writer)
{
    doors_->Sync([this, &writer](const GsxDoor door, const bool open)
    {
        writer.SetLVar(aircraft_->DoorModeLVar(door), open ? kDoorOpen : kDoorClosed);
    });
}
