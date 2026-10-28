#include "Fss727FrontEntryServesTheGroundAccessRule.h"

#include <QtCore/QString>

#include "../Fss727.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-front-entry-serves-the-ground-access";

    constexpr auto kFrontEntryGoal = "INTERACTIVE POINT GOAL:0";
    constexpr auto kPercentOver100Unit = "percent over 100";

    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayUnavailable = 2.0;
}

Fss727FrontEntryServesTheGroundAccessRule::Fss727FrontEntryServesTheGroundAccessRule(
    VariableReader& variables, const Fss727& aircraft, GsxDoorSync& doors)
    : variables_(&variables), aircraft_(&aircraft), doors_(&doors)
{
}

const char* Fss727FrontEntryServesTheGroundAccessRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727FrontEntryServesTheGroundAccessRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727FrontEntryServesTheGroundAccessRule::Act(const RuleContext&, VariableWriter& writer)
{
    doors_->Report();

    targetRestoredOpen_ = targetRestoredOpen_ && !HaveTheInputsArrived();

    const bool held = aircraft_->IsHeldForDeparture();
    const double target = IsFrontEntryWanted() ? kDoorOpen : kDoorClosed;
    const int closeRequests = aircraft_->FrontEntryCloseRequests();

    if (target == kDoorClosed && targetRestoredOpen_ && !held)
    {
        return;
    }

    if (target == kDoorClosed && closeRequests != servedCloseRequests_)
    {
        servedCloseRequests_ = closeRequests;
        CloseOnRequest(writer);

        return;
    }

    if (!held && !IsGsxRunning())
    {
        return;
    }

    const bool neverCommanded = lastTarget_ < kDoorClosed;

    if (target == lastTarget_ || (neverCommanded && target == kDoorClosed))
    {
        return;
    }

    FollowTheGroundAccess(writer, target);
}

void Fss727FrontEntryServesTheGroundAccessRule::CloseOnRequest(VariableWriter& writer)
{
    if (doors_->PositionOf(GsxDoor::FwdPax) == DoorPosition::Closed)
    {
        Record(kDoorClosed);

        return;
    }

    Command(writer, kDoorClosed);
}

void Fss727FrontEntryServesTheGroundAccessRule::FollowTheGroundAccess(VariableWriter& writer, const double target)
{
    const DoorPosition position = doors_->PositionOf(GsxDoor::FwdPax);
    if (position == DoorPosition::Pending)
    {
        return;
    }

    if (position == (target == kDoorOpen ? DoorPosition::Open : DoorPosition::Closed))
    {
        Record(target);

        return;
    }

    Command(writer, target);
}

void Fss727FrontEntryServesTheGroundAccessRule::Record(const double target)
{
    lastTarget_ = target;
    targetRestoredOpen_ = false;

    probe::Line(probe::Channel::Writes,
                QStringLiteral("skip  front FwdPax open=%1 already there").arg(target == kDoorOpen ? 1 : 0));
}

void Fss727FrontEntryServesTheGroundAccessRule::Command(VariableWriter& writer, const double target)
{
    lastTarget_ = target;
    targetRestoredOpen_ = false;

    probe::Line(probe::Channel::Writes, QStringLiteral("write front FwdPax open=%1").arg(target == kDoorOpen ? 1 : 0));
    writer.SetAVar(kFrontEntryGoal, kPercentOver100Unit, target);

    LOG_INFO("FSS 727 front entry door commanded %s", target == kDoorOpen ? "open" : "closed");
}

std::optional<double> Fss727FrontEntryServesTheGroundAccessRule::CommandedTarget() const
{
    if (lastTarget_ < kDoorClosed)
    {
        return std::nullopt;
    }

    return lastTarget_;
}

bool Fss727FrontEntryServesTheGroundAccessRule::HasUnservedClose() const
{
    return aircraft_->FrontEntryCloseRequests() != servedCloseRequests_;
}

void Fss727FrontEntryServesTheGroundAccessRule::RestoreTarget(const double target)
{
    lastTarget_ = target;
    targetRestoredOpen_ = target == kDoorOpen;
}

void Fss727FrontEntryServesTheGroundAccessRule::ForgetTheRestoredTarget()
{
    targetRestoredOpen_ = false;
}

bool Fss727FrontEntryServesTheGroundAccessRule::HaveTheInputsArrived() const
{
    return variables_->HasReceivedLVar(gsx::lvars::kCouatlStarted)
        && variables_->HasReceivedLVar(gsx::lvars::kJetway)
        && variables_->HasReceivedLVar(gsx::lvars::kPassengerStairsFrontState);
}

bool Fss727FrontEntryServesTheGroundAccessRule::IsGsxRunning() const
{
    return variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) >= 1.0;
}

bool Fss727FrontEntryServesTheGroundAccessRule::IsFrontEntryWanted() const
{
    if (aircraft_->IsHeldForDeparture())
    {
        return false;
    }

    if (doors_->VehicleState(gsx::lvars::kJetway, kJetwayUnavailable) == kJetwayDocked)
    {
        return true;
    }

    return gsx::states::AreStairsArriving(
        doors_->VehicleState(gsx::lvars::kPassengerStairsFrontState, 0.0));
}
