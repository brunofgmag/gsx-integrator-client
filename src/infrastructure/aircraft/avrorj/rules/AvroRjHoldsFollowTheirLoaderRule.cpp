#include "AvroRjHoldsFollowTheirLoaderRule.h"

#include <QtCore/QString>

#include "../AvroRj.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "avro-rj-holds-follow-their-loader";

    constexpr auto kFwdHoldName = "forward hold";
    constexpr auto kAftHoldName = "aft hold";
    constexpr auto kFwdHoldDoorLVar = "EXT_Door_cargo_fwd";
    constexpr auto kAftHoldDoorLVar = "EXT_Door_cargo_aft";

    constexpr int kAircraftHeadStartTicks = 5;

    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;

    constexpr auto kLoaderIsAtTheHold = "its baggage loader is at the door";
    constexpr auto kNoLoaderIsAtTheHold = "no baggage loader is at the door";
}

AvroRjHoldsFollowTheirLoaderRule::AvroRjHoldsFollowTheirLoaderRule(VariableReader& variables,
                                                                  const AvroRj& aircraft,
                                                                  GsxDoorSync& doors)
    : variables_(&variables),
      aircraft_(&aircraft),
      doors_(&doors),
      holds_{{{.name = kFwdHoldName,
               .doorLVar = kFwdHoldDoorLVar,
               .loaderLVar = gsx::lvars::kBaggageLoaderFrontState},
              {.name = kAftHoldName,
               .doorLVar = kAftHoldDoorLVar,
               .loaderLVar = gsx::lvars::kBaggageLoaderRearState}}}
{
}

const char* AvroRjHoldsFollowTheirLoaderRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjHoldsFollowTheirLoaderRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void AvroRjHoldsFollowTheirLoaderRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    for (Hold& hold : holds_)
    {
        Follow(hold, writer);
    }
}

bool AvroRjHoldsFollowTheirLoaderRule::IsLoaderServing(const Hold& hold) const
{
    return !aircraft_->IsHeldForDeparture()
        && gsx::states::IsLoaderServingTheDoor(doors_->VehicleState(hold.loaderLVar, 0.0));
}

void AvroRjHoldsFollowTheirLoaderRule::Follow(Hold& hold, VariableWriter& writer)
{
    if (!variables_->HasReceivedLVar(hold.doorLVar))
    {
        return;
    }

    const bool isOpen = variables_->GetLVar(hold.doorLVar, 0.0) == kDoorOpen;
    const bool wantedOpen = IsLoaderServing(hold);

    if (isOpen == wantedOpen)
    {
        hold.pendingTarget.reset();

        return;
    }

    if (hold.pendingTarget != wantedOpen)
    {
        hold.pendingTarget = wantedOpen;
        hold.pendingTicks = 0;
        hold.commanded = false;
    }

    if (hold.commanded || ++hold.pendingTicks < kAircraftHeadStartTicks)
    {
        return;
    }

    hold.commanded = true;
    Write(hold, wantedOpen, wantedOpen ? kLoaderIsAtTheHold : kNoLoaderIsAtTheHold, writer);
}

void AvroRjHoldsFollowTheirLoaderRule::Write(const Hold& hold, const bool open, const char* const reason,
                                             VariableWriter& writer)
{
    probe::Line(probe::Channel::Writes,
                QStringLiteral("write %1 open=%2").arg(QString::fromUtf8(hold.name)).arg(open ? 1 : 0));
    LOG_INFO("%s the %s: %s", open ? "Opening" : "Closing", hold.name, reason);
    writer.SetLVar(hold.doorLVar, open ? kDoorOpen : kDoorClosed);
}
