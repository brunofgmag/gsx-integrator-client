#include "TfdiMd11PaxDoorsFollowStairsRule.h"

#include "../TfdiMd11.h"
#include "../../DoorReading.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-pax-doors-follow-stairs";

    constexpr auto kPaxDoor1LLVar = "MD11_EXT_DOOR_CMD_PAX_1L";
    constexpr auto kPaxDoor2LLVar = "MD11_EXT_DOOR_CMD_PAX_2L";
    constexpr auto kPaxDoor4LLVar = "MD11_EXT_DOOR_CMD_PAX_4L";

    constexpr auto kPaxDoor1LReadingLVar = "MD11_EXT_DOOR_PAX_1L";
    constexpr auto kPaxDoor2LReadingLVar = "MD11_EXT_DOOR_PAX_2L";
    constexpr auto kPaxDoor4LReadingLVar = "MD11_EXT_DOOR_PAX_4L";

    constexpr double kDoorOpen = 100.0;
    constexpr double kDoorClosed = 0.0;
}

TfdiMd11PaxDoorsFollowStairsRule::TfdiMd11PaxDoorsFollowStairsRule(VariableReader& variables,
                                                                  const TfdiMd11& aircraft)
    : variables_(&variables), aircraft_(&aircraft)
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

    FollowStairs(writer, {.stairsState = gsx::lvars::kPassengerStairsFrontState, .command = kPaxDoor1LLVar, .reading = kPaxDoor1LReadingLVar}, targets_.fwd);
    FollowStairs(writer, {.stairsState = gsx::lvars::kPassengerStairsMiddleState, .command = kPaxDoor2LLVar, .reading = kPaxDoor2LReadingLVar}, targets_.mid);
    FollowStairs(writer, {.stairsState = gsx::lvars::kPassengerStairsRearState, .command = kPaxDoor4LLVar, .reading = kPaxDoor4LReadingLVar}, targets_.aft);
}

TfdiMd11PaxDoorsFollowStairsRule::DoorTargets TfdiMd11PaxDoorsFollowStairsRule::Targets() const
{
    return targets_;
}

void TfdiMd11PaxDoorsFollowStairsRule::RestoreTargets(const DoorTargets& targets)
{
    targets_ = targets;
}

void TfdiMd11PaxDoorsFollowStairsRule::FollowStairs(VariableWriter& writer, const DoorLVars& door,
                                                    std::optional<double>& lastDoorTarget) const
{
    if (!variables_->HasReceivedLVar(door.stairsState))
    {
        return;
    }

    if (aircraft_->ArePassengerDoorsHeld())
    {
        CloseWhileHeld(writer, door, lastDoorTarget);

        return;
    }

    if (gsx::states::AreStairsArriving(variables_->GetLVar(door.stairsState, 0.0)))
    {
        CommandWhenTheDoorDiffers(writer, door, kDoorOpen, lastDoorTarget);
    }
    else if (lastDoorTarget == kDoorOpen)
    {
        CommandWhenTheDoorDiffers(writer, door, kDoorClosed, lastDoorTarget);
    }
}

void TfdiMd11PaxDoorsFollowStairsRule::CloseWhileHeld(VariableWriter& writer, const DoorLVars& door,
                                                      std::optional<double>& lastDoorTarget) const
{
    if (lastDoorTarget == kDoorOpen)
    {
        CommandWhenTheDoorDiffers(writer, door, kDoorClosed, lastDoorTarget);
    }
}

void TfdiMd11PaxDoorsFollowStairsRule::CommandWhenTheDoorDiffers(VariableWriter& writer, const DoorLVars& door,
                                                                 const double target,
                                                                 std::optional<double>& lastDoorTarget) const
{
    if (lastDoorTarget == target)
    {
        return;
    }

    const std::optional<bool> isOpen = doors::OpenAboveZero(*variables_, door.reading);

    if (!isOpen.has_value())
    {
        return;
    }

    const bool wantsOpen = target == kDoorOpen;

    if (*isOpen != wantsOpen)
    {
        writer.SetLVar(door.command, target);
    }

    lastDoorTarget = target;
}
