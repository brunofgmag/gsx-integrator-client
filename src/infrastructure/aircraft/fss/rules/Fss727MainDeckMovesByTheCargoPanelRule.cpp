#include "Fss727MainDeckMovesByTheCargoPanelRule.h"

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

    bool IsUnderway(const GsxStateStatus state)
    {
        return state == GsxStateStatus::Requested || state == GsxStateStatus::Active;
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
}

void Fss727MainDeckMovesByTheCargoPanelRule::FinishTravel(VariableWriter& writer)
{
    if (travel_ == Travel::Opening)
    {
        if (!aircraft_->IsMainDeckOpen().value_or(false))
        {
            return;
        }

        TurnThePanelMasterOff(writer);

        LOG_INFO("FSS 727 main deck door open: the cargo panel master goes off");

        return;
    }

    if (!aircraft_->IsMainDeckClosed().value_or(false))
    {
        return;
    }

    TurnThePanelMasterOff(writer);
    servedRequests_ = aircraft_->MainDeckCloseRequests();

    LOG_INFO("FSS 727 main deck door closed: the cargo panel master goes off");
}

void Fss727MainDeckMovesByTheCargoPanelRule::TurnThePanelMasterOff(VariableWriter& writer)
{
    probe::Line(QStringLiteral("write panel master=0"));
    writer.SetLVar(kMasterPowerLVar, kSwitchOff);
    travel_ = Travel::None;
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
    return IsGsxUnderway(GsxState::Boarding) || IsGsxUnderway(GsxState::Deboarding);
}

bool Fss727MainDeckMovesByTheCargoPanelRule::IsGsxUnderway(const GsxState state) const
{
    return gsxGateway_ != nullptr && IsUnderway(gsxGateway_->GetStateStatus(state));
}
