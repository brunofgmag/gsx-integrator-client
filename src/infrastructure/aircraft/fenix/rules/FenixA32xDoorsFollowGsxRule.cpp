#include "FenixA32xDoorsFollowGsxRule.h"

#include "../FenixA32x.h"
#include "../../../fenix/FenixEfbGateway.h"
#include "../../../gsx/GsxDoorSync.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-doors-follow-gsx";
}

FenixA32xDoorsFollowGsxRule::FenixA32xDoorsFollowGsxRule(const FenixA32x& aircraft, FenixEfbGateway& efb,
                                                        GsxDoorSync& doors, const FenixVariant variant)
    : aircraft_(&aircraft), efb_(&efb), doors_(&doors), variant_(variant)
{
}

const char* FenixA32xDoorsFollowGsxRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xDoorsFollowGsxRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xDoorsFollowGsxRule::Act(const RuleContext&, VariableWriter&)
{
    if (!efb_->IsAvailable())
    {
        return;
    }

    doors_->Sync([this](const GsxDoor door, const bool open)
    {
        if (door == GsxDoor::MidPax && variant_ != FenixVariant::A321)
        {
            return;
        }

        efb_->SetBool(aircraft_->DoorDataref(door), open);
    });
}
