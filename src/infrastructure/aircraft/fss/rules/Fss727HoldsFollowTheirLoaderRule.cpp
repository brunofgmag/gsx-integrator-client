#include "Fss727HoldsFollowTheirLoaderRule.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <QtCore/QString>

#include "../Fss727.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-holds-follow-their-loader";

    constexpr auto kPercentOver100Unit = "percent over 100";
    constexpr double kHoldGoalOpen = 1.0;
    constexpr double kHoldGoalClosed = 0.0;

    struct Hold
    {
        GsxDoor door;
        const char* name;
        const char* goal;
        const char* loaderLVar;
    };

    constexpr std::array kHolds = {
        Hold{.door = GsxDoor::FwdCargo, .name = "forward", .goal = "INTERACTIVE POINT GOAL:2",
             .loaderLVar = gsx::lvars::kBaggageLoaderFrontState},
        Hold{.door = GsxDoor::AftCargo, .name = "aft", .goal = "INTERACTIVE POINT GOAL:3",
             .loaderLVar = gsx::lvars::kBaggageLoaderRearState}
    };

    static_assert(kHolds.size() == Fss727HoldsFollowTheirLoaderRule::kHoldCount);
}

Fss727HoldsFollowTheirLoaderRule::Fss727HoldsFollowTheirLoaderRule(
    VariableReader& variables, const Fss727& aircraft, GsxDoorSync& doors)
    : variables_(&variables), aircraft_(&aircraft), doors_(&doors)
{
}

const char* Fss727HoldsFollowTheirLoaderRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727HoldsFollowTheirLoaderRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727HoldsFollowTheirLoaderRule::Act(const RuleContext&, VariableWriter& writer)
{
    doors_->Sync([&writer](const GsxDoor door, const bool open) { MoveTheHold(writer, door, open); });
    CloseTheHoldsOnRequest(writer);
}

void Fss727HoldsFollowTheirLoaderRule::MoveTheHold(VariableWriter& writer, const GsxDoor door, const bool open)
{
    const auto hold = std::ranges::find(kHolds, door, &Hold::door);
    if (hold == kHolds.end())
    {
        return;
    }

    probe::Line(probe::Channel::Writes,
                QStringLiteral("write hold %1 %2=%3").arg(QLatin1String(hold->name), QLatin1String(hold->goal)).arg(open ? 1 : 0));
    writer.SetAVar(hold->goal, kPercentOver100Unit, open ? kHoldGoalOpen : kHoldGoalClosed);

    LOG_INFO("FSS 727 %s hold commanded %s: its loader %s the door", hold->name, open ? "open" : "closed",
             open ? "has reached" : "has left");
}

void Fss727HoldsFollowTheirLoaderRule::CloseTheHoldsOnRequest(VariableWriter& writer)
{
    const int requests = aircraft_->HoldCloseRequests();

    for (std::size_t index = 0; index < kHolds.size(); ++index)
    {
        const Hold& hold = kHolds[index];
        int& served = servedRequests_[index];

        if (served == requests || !HasItsLoaderLeft(hold.loaderLVar))
        {
            continue;
        }

        probe::Line(probe::Channel::Writes, QStringLiteral("write hold %1 %2=0").arg(QLatin1String(hold.name), QLatin1String(hold.goal)));
        writer.SetAVar(hold.goal, kPercentOver100Unit, kHoldGoalClosed);
        served = requests;

        LOG_INFO("FSS 727 %s hold commanded closed: its loader has left", hold.name);
    }
}

bool Fss727HoldsFollowTheirLoaderRule::HasUnservedClose(const std::size_t hold) const
{
    return servedRequests_[hold] != aircraft_->HoldCloseRequests();
}

void Fss727HoldsFollowTheirLoaderRule::RestoreUnservedCloses(const std::array<bool, kHoldCount>& unserved)
{
    for (std::size_t hold = 0; hold < kHoldCount; ++hold)
    {
        servedRequests_[hold] = unserved[hold] ? 0 : aircraft_->HoldCloseRequests();
    }
}

bool Fss727HoldsFollowTheirLoaderRule::HasItsLoaderLeft(const char* loaderLVar) const
{
    return variables_->HasReceivedLVar(loaderLVar)
        && !gsx::states::IsLoaderServingTheDoor(doors_->VehicleState(loaderLVar, 0.0));
}
