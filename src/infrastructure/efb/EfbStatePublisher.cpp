#include "EfbStatePublisher.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "../../viewmodel/OperationsViewModel.h"

EfbStatePublisher::EfbStatePublisher(CommBusBridgeGateway* bridge, const OperationsViewModel* view,
                                     std::function<SimVersion()> simVersion)
    : bridge_(bridge), view_(view), simVersion_(std::move(simVersion))
{
}

void EfbStatePublisher::Publish()
{
    if (simVersion_() != SimVersion::Msfs2024)
    {
        return;
    }

    if (!bridge_->IsAvailable())
    {
        return;
    }

    std::string payload = BuildPayload();
    if (payload == lastPayload_)
    {
        return;
    }

    if (bridge_->Call(EfbCommBus::kStateChannel, CommBusFlag::kJs, payload))
    {
        lastPayload_ = std::move(payload);
    }
}

std::string EfbStatePublisher::BuildPayload() const
{
    QJsonObject state;

    state.insert(QLatin1String("connected"), view_->IsConnected());
    state.insert(QLatin1String("enabled"), view_->IsEnabled());
    state.insert(QLatin1String("gsxAvailable"), view_->IsGsxAvailable());
    state.insert(QLatin1String("aircraftSupported"), view_->IsAircraftSupported());
    state.insert(QLatin1String("aircraftNameText"), view_->GetAircraftNameText());

    state.insert(QLatin1String("phase"), view_->GetPhase());
    state.insert(QLatin1String("phaseCount"), OperationsViewModel::GetPhaseCount());
    state.insert(QLatin1String("stateText"), view_->GetStateText());
    state.insert(QLatin1String("phaseTip"), view_->GetPhaseTip());
    state.insert(QLatin1String("nextPhaseText"), view_->GetNextPhaseText());
    state.insert(QLatin1String("holdCountdownText"), view_->GetHoldCountdownText());
    state.insert(QLatin1String("advancedByPilot"), view_->AdvancedByPilot());
    state.insert(QLatin1String("inDeboardingPhase"), view_->IsInDeboardingPhase());
    state.insert(QLatin1String("cargoAircraft"), view_->IsCargoAircraft());

    state.insert(QLatin1String("fuelProgress"), view_->GetFuelProgress());
    state.insert(QLatin1String("boardingProgress"), view_->GetBoardingProgress());
    state.insert(QLatin1String("deboardingProgress"), view_->GetDeboardingProgress());
    state.insert(QLatin1String("loadedFuelText"), view_->GetLoadedFuelText());
    state.insert(QLatin1String("targetFuelText"), view_->GetTargetFuelText());
    state.insert(QLatin1String("targetZfwText"), view_->GetTargetZfwText());
    state.insert(QLatin1String("plannedFuelText"), view_->GetPlannedFuelText());
    state.insert(QLatin1String("plannedZfwText"), view_->GetPlannedZfwText());

    state.insert(QLatin1String("plannedPax"), view_->GetPlannedPax());
    state.insert(QLatin1String("boardedPax"), view_->GetBoardedPax());
    state.insert(QLatin1String("deboardedPax"), view_->GetDeboardedPax());
    state.insert(QLatin1String("targetPax"), view_->GetTargetPax());

    state.insert(QLatin1String("simbriefStatusText"), view_->GetSimbriefStatusText());
    state.insert(QLatin1String("simbriefRefusal"), view_->GetSimbriefRefusal());
    state.insert(QLatin1String("simbriefReady"), view_->IsSimbriefReady());
    state.insert(QLatin1String("simbriefError"), view_->HasSimbriefError());

    state.insert(QLatin1String("gsxProfileConflict"), view_->HasGsxProfileConflict());
    state.insert(QLatin1String("gsxProfileFixable"), view_->IsGsxProfileFixable());
    state.insert(QLatin1String("pmdgOptionsConflict"), view_->HasPmdgOptionsConflict());
    state.insert(QLatin1String("pmdgOptionsFixable"), view_->IsPmdgOptionsFixable());
    state.insert(QLatin1String("cargoDoorStuck"), view_->IsCargoDoorStuck());
    state.insert(QLatin1String("fuelRequestStalled"), view_->IsFuelRequestStalled());

    state.insert(QLatin1String("canToggleAutomation"), view_->CanToggleAutomation());
    state.insert(QLatin1String("canStartLoading"), view_->CanStartLoading());
    state.insert(QLatin1String("canReloadSimbrief"), view_->CanReloadSimbrief());
    state.insert(QLatin1String("commandError"), view_->GetCommandError());

    return QJsonDocument(state).toJson(QJsonDocument::Compact).toStdString();
}
