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

Fss727MainDeckMovesByTheCargoPanelRule::Fss727MainDeckMovesByTheCargoPanelRule(
    const Fss727& aircraft, const GsxGateway* gsxGateway, const GsxDoorSync& doors)
    : aircraft_(&aircraft), gsxGateway_(gsxGateway), doors_(&doors)
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

    if (IsCloseRequestServable())
    {
        if (*closed)
        {
            servedRequests_ = aircraft_->MainDeckCloseRequests();

            return;
        }

        StartTravel(writer, Travel::Closing);

        LOG_INFO("FSS 727 main deck door commanded closed, and the panel master stays on for the whole travel");

        return;
    }

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
    watch_ = TravelWatch{.lastPosition = aircraft_->MainDeckPosition()};
}

void Fss727MainDeckMovesByTheCargoPanelRule::FinishTravel(VariableWriter& writer)
{
    const std::optional<double> position = aircraft_->MainDeckPosition();
    if (!position.has_value())
    {
        return;
    }

    Follow(*position);
    if (!HasComeToRest())
    {
        return;
    }

    if (travel_ == Travel::Closing)
    {
        servedRequests_ = aircraft_->MainDeckCloseRequests();
    }

    travel_ = Travel::None;

    if (!IsAtAnEnd(*position))
    {
        LOG_INFO("FSS 727 main deck door came to rest at %.1f%%, short of both ends, so the cargo panel master is left as it is",
                 *position * kPercentPerFraction);

        return;
    }

    TurnThePanelMasterOff(writer, *position);
}

void Fss727MainDeckMovesByTheCargoPanelRule::Follow(const double position)
{
    if (watch_.lastPosition.has_value() && std::abs(position - *watch_.lastPosition) <= kStillWithin)
    {
        ++watch_.stillTicks;
    }
    else
    {
        watch_.moved = watch_.moved || watch_.lastPosition.has_value();
        watch_.stillTicks = 0;
    }

    watch_.lastPosition = position;
}

bool Fss727MainDeckMovesByTheCargoPanelRule::HasComeToRest() const
{
    return watch_.moved && watch_.stillTicks >= kRestingTicks;
}

void Fss727MainDeckMovesByTheCargoPanelRule::TurnThePanelMasterOff(VariableWriter& writer, const double position)
{
    probe::Line(QStringLiteral("write panel master=0 rest=%1").arg(position, 0, 'f', kProbePositionDecimals));
    writer.SetLVar(kMasterPowerLVar, kSwitchOff);

    LOG_INFO("FSS 727 main deck door at rest at %.1f%%: the cargo panel master goes off", position * kPercentPerFraction);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsCloseRequestServable() const
{
    return aircraft_->MainDeckCloseRequests() != servedRequests_ && !IsGsxWorkingTheCargoDoors();
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsTheMainLoaderWaitingForTheDeck() const
{
    return !aircraft_->IsHeldForDeparture()
        && IsGsxUnderway(GsxState::Boarding)
        && doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0) == gsx::states::kLoaderWaitingForDoor;
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxWorkingTheCargoDoors() const
{
    return IsGsxWorkingTheDoors(GsxState::Boarding) || IsGsxWorkingTheDoors(GsxState::Deboarding);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxUnderway(const GsxState state) const
{
    return gsxGateway_ != nullptr && IsUnderway(gsxGateway_->GetStateStatus(state));
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxWorkingTheDoors(const GsxState state) const
{
    return gsxGateway_ != nullptr && IsWorkingTheDoors(gsxGateway_->GetStateStatus(state));
}
