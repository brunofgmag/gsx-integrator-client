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

    const double target = IsFrontEntryWanted() ? kDoorOpen : kDoorClosed;
    const int closeRequests = aircraft_->FrontEntryCloseRequests();

    if (target == kDoorClosed && closeRequests != servedCloseRequests_)
    {
        servedCloseRequests_ = closeRequests;
        Command(writer, kDoorClosed);

        return;
    }

    const bool neverCommanded = lastTarget_ < kDoorClosed;

    if (target == lastTarget_ || (neverCommanded && target == kDoorClosed))
    {
        return;
    }

    Command(writer, target);
}

void Fss727FrontEntryServesTheGroundAccessRule::Command(VariableWriter& writer, const double target)
{
    lastTarget_ = target;

    probe::Line(probe::Channel::Writes, QStringLiteral("write front FwdPax open=%1").arg(target == kDoorOpen ? 1 : 0));
    writer.SetAVar(kFrontEntryGoal, kPercentOver100Unit, target);

    LOG_INFO("FSS 727 front entry door commanded %s", target == kDoorOpen ? "open" : "closed");
}

bool Fss727FrontEntryServesTheGroundAccessRule::IsFrontEntryWanted() const
{
    if (aircraft_->IsHeldForDeparture())
    {
        return false;
    }

    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
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
