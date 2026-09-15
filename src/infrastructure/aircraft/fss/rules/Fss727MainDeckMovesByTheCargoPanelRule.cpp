#include "Fss727MainDeckMovesByTheCargoPanelRule.h"

#include <cmath>
#include <optional>
#include <QtCore/QString>

#include "../Fss727.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/ports/GsxGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-main-deck-moves-by-the-cargo-panel";

    constexpr auto kMasterCoverLVar = "FSS_B727_CDP_MASTER_POWER_COVER_SWITCH";
    constexpr auto kMasterPowerLVar = "FSS_B727_CDP_MASTER_POWER_SWITCH";
    constexpr auto kCargoDoorSwitchLVar = "FSS_B727_CDP_CARGO_DOOR_SWITCH";

    constexpr double kSwitchOn = 1.0;
    constexpr double kSwitchOff = 0.0;

    constexpr int kRestingTicks = 3;
    constexpr int kMasterCutGuardTicks = 3;
    constexpr int kMainLoaderGiveUpTicks = 120;
    constexpr double kStillWithin = 0.001;
    constexpr double kRestsClosedAtMost = 0.02;
    constexpr double kRestsOpenAtLeast = 0.98;
    constexpr double kPercentPerFraction = 100.0;
    constexpr int kProbePositionDecimals = 4;

    bool IsUnderway(const GsxStateStatus state)
    {
        return state == GsxStateStatus::Requested || state == GsxStateStatus::Active;
    }

    bool IsWorkingTheDoors(const GsxStateStatus state)
    {
        return IsUnderway(state) || state == GsxStateStatus::Completing;
    }

    bool IsAtAnEnd(const double position)
    {
        return position <= kRestsClosedAtMost || position >= kRestsOpenAtLeast;
    }
}

void Fss727DoorRest::Follow(const double position)
{
    if (lastPosition.has_value() && std::abs(position - *lastPosition) <= kStillWithin)
    {
        ++stillTicks;
    }
    else
    {
        moved = moved || lastPosition.has_value();
        stillTicks = 0;
    }

    lastPosition = position;
}

bool Fss727DoorRest::HasMoved() const
{
    return moved;
}

bool Fss727DoorRest::IsStill() const
{
    return stillTicks >= kRestingTicks;
}

Fss727MainDeckMovesByTheCargoPanelRule::Fss727MainDeckMovesByTheCargoPanelRule(
    VariableReader& variables, const Fss727& aircraft, const GsxGateway* gsxGateway, const GsxDoorSync& doors)
    : variables_(&variables), aircraft_(&aircraft), gsxGateway_(gsxGateway), doors_(&doors)
{
}

const char* Fss727MainDeckMovesByTheCargoPanelRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727MainDeckMovesByTheCargoPanelRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727MainDeckMovesByTheCargoPanelRule::Act(const RuleContext&, VariableWriter& writer)
{
    GuardThePanelMasterCut(writer);

    if (travel_ != Travel::None)
    {
        FinishTravel(writer);

        return;
    }

    const std::optional<bool> closed = aircraft_->IsMainDeckClosed();
    if (!closed.has_value())
    {
        return;
    }

    if (IsCloseRequestPending())
    {
        ServeThePendingClose(writer, *closed);

        return;
    }

    loaderHoldTicks_ = 0;

    if (!*closed || !IsTheMainLoaderWaitingForTheDeck())
    {
        return;
    }

    StartTravel(writer, Travel::Opening);

    LOG_INFO("FSS 727 main deck door commanded open for the main loader waiting on it, and the panel master stays on for the whole travel");
}

void Fss727MainDeckMovesByTheCargoPanelRule::StartTravel(VariableWriter& writer, const Travel travel)
{
    const bool opening = travel == Travel::Opening;

    probe::Line(QStringLiteral("write panel cover=1 master=1 door=%1").arg(opening ? 1 : 0));
    writer.SetLVar(kMasterCoverLVar, kSwitchOn);
    writer.SetLVar(kMasterPowerLVar, kSwitchOn);
    writer.SetLVar(kCargoDoorSwitchLVar, opening ? kSwitchOn : kSwitchOff);
    travel_ = travel;
    masterCutGuardTicks_ = 0;
    mayResumeTravel_ = true;
    rest_ = Fss727DoorRest{.lastPosition = aircraft_->MainDeckPosition()};
}

void Fss727MainDeckMovesByTheCargoPanelRule::ServeThePendingClose(VariableWriter& writer, const bool closed)
{
    const bool loaderLeft = HasTheMainLoaderLeft();
    if (!loaderLeft && ++loaderHoldTicks_ < kMainLoaderGiveUpTicks)
    {
        return;
    }

    loaderHoldTicks_ = 0;

    if (closed)
    {
        servedRequests_ = aircraft_->MainDeckCloseRequests();

        return;
    }

    StartTravel(writer, Travel::Closing);

    if (loaderLeft)
    {
        LOG_INFO("FSS 727 main deck door commanded closed, and the panel master stays on for the whole travel");

        return;
    }

    LOG_INFO("FSS 727 main deck door commanded closed with the main loader still in place after %d ticks, because an open deck holds the pushback gate",
             kMainLoaderGiveUpTicks);
}

void Fss727MainDeckMovesByTheCargoPanelRule::FinishTravel(VariableWriter& writer)
{
    const std::optional<double> position = aircraft_->MainDeckPosition();
    if (!position.has_value())
    {
        return;
    }

    rest_.Follow(*position);
    if (!HasComeToRest())
    {
        return;
    }

    if (travel_ == Travel::Closing)
    {
        servedRequests_ = aircraft_->MainDeckCloseRequests();
    }

    cutTravel_ = travel_;
    travel_ = Travel::None;

    if (!IsAtAnEnd(*position))
    {
        LOG_INFO("FSS 727 main deck door came to rest at %.1f%%, short of both ends, so the cargo panel master is left as it is",
                 *position * kPercentPerFraction);

        return;
    }

    TurnThePanelMasterOff(writer, *position);
}

void Fss727MainDeckMovesByTheCargoPanelRule::ResumeTravel(const double position)
{
    mayResumeTravel_ = false;
    travel_ = cutTravel_;
    rest_ = Fss727DoorRest{.lastPosition = position};
}

void Fss727MainDeckMovesByTheCargoPanelRule::GuardThePanelMasterCut(VariableWriter& writer)
{
    if (masterCutGuardTicks_ <= 0)
    {
        return;
    }

    const std::optional<double> position = aircraft_->MainDeckPosition();
    if (!position.has_value())
    {
        return;
    }

    --masterCutGuardTicks_;

    if (IsAtAnEnd(*position))
    {
        return;
    }

    masterCutGuardTicks_ = 0;

    probe::Line(QStringLiteral("write panel master=1 resume=%1").arg(*position, 0, 'f', kProbePositionDecimals));
    writer.SetLVar(kMasterPowerLVar, kSwitchOn);
    ResumeTravel(*position);

    LOG_INFO("FSS 727 main deck door reads %.1f%% right after the cargo panel master went off: the master goes back on and the travel goes back to the rule",
             *position * kPercentPerFraction);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::HasComeToRest() const
{
    return rest_.HasMoved() && rest_.IsStill();
}

void Fss727MainDeckMovesByTheCargoPanelRule::TurnThePanelMasterOff(VariableWriter& writer, const double position)
{
    probe::Line(QStringLiteral("write panel master=0 rest=%1").arg(position, 0, 'f', kProbePositionDecimals));
    writer.SetLVar(kMasterPowerLVar, kSwitchOff);
    masterCutGuardTicks_ = mayResumeTravel_ ? kMasterCutGuardTicks : 0;

    LOG_INFO("FSS 727 main deck door at rest at %.1f%%: the cargo panel master goes off", position * kPercentPerFraction);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsCloseRequestPending() const
{
    return aircraft_->MainDeckCloseRequests() != servedRequests_ && !IsGsxWorkingTheCargoDoors();
}

bool Fss727MainDeckMovesByTheCargoPanelRule::HasTheMainLoaderLeft() const
{
    return variables_->HasReceivedLVar(gsx::lvars::kBaggageLoaderMainState)
        && doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0) < gsx::states::kVehicleDispatched;
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsTheMainLoaderWaitingForTheDeck() const
{
    return !aircraft_->IsHeldForDeparture()
        && (IsGsxUnderway(GsxState::Boarding) || IsGsxUnderway(GsxState::Deboarding))
        && doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0) == gsx::states::kLoaderWaitingForDoor;
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxWorkingTheCargoDoors() const
{
    return IsGsxWorkingTheDoors(GsxState::Boarding) || IsGsxWorkingTheDoors(GsxState::Deboarding);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxUnderway(const GsxState state) const
{
    return IsUnderway(GsxStatusOf(state));
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxWorkingTheDoors(const GsxState state) const
{
    return IsWorkingTheDoors(GsxStatusOf(state));
}

GsxStateStatus Fss727MainDeckMovesByTheCargoPanelRule::GsxStatusOf(const GsxState state) const
{
    return gsxGateway_ != nullptr ? gsxGateway_->GetStateStatus(state) : GsxStateStatus::Unavailable;
}
