#include "TfdiMd11PaxDoorsFollowStairsRule.h"

#include "../../../gsx/GsxLVars.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-pax-doors-follow-stairs";

    constexpr auto kPaxDoor1LLVar = "MD11_EXT_DOOR_CMD_PAX_1L";
    constexpr auto kPaxDoor2LLVar = "MD11_EXT_DOOR_CMD_PAX_2L";
    constexpr auto kPaxDoor4LLVar = "MD11_EXT_DOOR_CMD_PAX_4L";

    constexpr double kDoorOpen = 100.0;
    constexpr double kDoorClosed = 0.0;
}

TfdiMd11PaxDoorsFollowStairsRule::TfdiMd11PaxDoorsFollowStairsRule(VariableReader& variables)
    : variables_(&variables)
{
}

const char* TfdiMd11PaxDoorsFollowStairsRule::Name() const
{
    return kRuleName;
}

RuleVerdict TfdiMd11PaxDoorsFollowStairsRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11PaxDoorsFollowStairsRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    FollowStairs(writer, gsx::lvars::kPassengerStairsFrontState, kPaxDoor1LLVar, fwdDoorTarget_);
    FollowStairs(writer, gsx::lvars::kPassengerStairsMiddleState, kPaxDoor2LLVar, midDoorTarget_);
    FollowStairs(writer, gsx::lvars::kPassengerStairsRearState, kPaxDoor4LLVar, aftDoorTarget_);
}

void TfdiMd11PaxDoorsFollowStairsRule::FollowStairs(VariableWriter& writer, const char* stairsStateLVar,
                                                    const char* doorCmdLVar, double& lastDoorTarget) const
{
    if (gsx::states::AreStairsArriving(variables_->GetLVar(stairsStateLVar, 0.0)))
    {
        if (lastDoorTarget != kDoorOpen)
        {
            writer.SetLVar(doorCmdLVar, kDoorOpen);
            lastDoorTarget = kDoorOpen;
        }
    }
    else if (lastDoorTarget == kDoorOpen)
    {
        writer.SetLVar(doorCmdLVar, kDoorClosed);
        lastDoorTarget = kDoorClosed;
    }
}
