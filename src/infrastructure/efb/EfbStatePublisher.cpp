#include "EfbStatePublisher.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "../../viewmodel/OperationsViewModel.h"

EfbStatePublisher::EfbStatePublisher(CommBusBridgeGateway* bridge, const OperationsViewModel* view,
                                     std::function<SimVersion()> simVersion)
    : bridge_(bridge), view_(view), simVersion_(std::move(simVersion))
{
}

void EfbStatePublisher::Setup()
{
    bridge_->Subscribe(EfbCommBus::kHelloChannel, CommBusFlag::kJs,
                       [this](const std::string&) { Republish(); });
}

bool EfbStatePublisher::CanPublish() const
{
    return simVersion_() == SimVersion::Msfs2024 && bridge_->IsAvailable();
}

void EfbStatePublisher::Send(std::string payload)
{
    if (bridge_->Call(EfbCommBus::kStateChannel, CommBusFlag::kJs, payload))
    {
        lastPayload_ = std::move(payload);
    }
}

void EfbStatePublisher::Republish()
{
    lastPayload_.clear();
    Publish();
}

void EfbStatePublisher::Publish()
{
    if (!CanPublish())
    {
        return;
    }

    std::string payload = BuildPayload();
    if (payload == lastPayload_)
    {
        return;
    }

    Send(std::move(payload));
}

void EfbStatePublisher::PublishDeparture()
{
    if (!CanPublish())
    {
        return;
    }

    Send(R"({"connected":false})");
}

std::string EfbStatePublisher::BuildPayload() const
{
    QJsonObject state;

    state.insert(QLatin1String("connected"), view_->IsConnected());
    state.insert(QLatin1String("enabled"), view_->IsEnabled());
    state.insert(QLatin1String("gsxAvailable"), view_->IsGsxAvailable());
    state.insert(QLatin1String("aircraftSupported"), view_->IsAircraftSupported());
    state.insert(QLatin1String("aircraftNameText"), view_->GetAircraftNameText());
    state.insert(QLatin1String("simLabel"), OperationsViewModel::GetSimLabel());
    state.insert(QLatin1String("simStatusText"), view_->GetSimStatusText());
    state.insert(QLatin1String("gsxLabel"), OperationsViewModel::GetGsxLabel());
    state.insert(QLatin1String("gsxStatusText"), view_->GetGsxStatusText());
    state.insert(QLatin1String("aircraftLabel"), OperationsViewModel::GetAircraftLabel());
    state.insert(QLatin1String("turnaroundModeLabel"), OperationsViewModel::GetTurnaroundModeLabel());
    state.insert(QLatin1String("turnaroundModeText"), view_->GetTurnaroundModeText());
    state.insert(QLatin1String("loadingModeLabel"), OperationsViewModel::GetLoadingModeLabel());
    state.insert(QLatin1String("loadingModeText"), view_->GetLoadingModeText());
    state.insert(QLatin1String("autoStartLoading"), view_->AutoStartsLoading());
    state.insert(QLatin1String("autoStartFlow"), view_->AutoStartsFlow());
    state.insert(QLatin1String("loadingRunning"), view_->IsLoadingRunning());

    state.insert(QLatin1String("phase"), view_->GetPhase());
    state.insert(QLatin1String("phaseCount"), OperationsViewModel::GetPhaseCount());
    state.insert(QLatin1String("turnaroundStateLabel"), OperationsViewModel::GetTurnaroundStateLabel());
    state.insert(QLatin1String("stateText"), view_->GetStateText());
    state.insert(QLatin1String("phaseCounterText"), view_->GetPhaseCounterText());
    state.insert(QLatin1String("advancedByPilotText"), OperationsViewModel::GetAdvancedByPilotText());
    state.insert(QLatin1String("phaseTip"), view_->GetPhaseTip());
    state.insert(QLatin1String("nextPhaseText"), view_->GetNextPhaseText());
    state.insert(QLatin1String("holdCountdownText"), view_->GetHoldCountdownText());
    state.insert(QLatin1String("advancedByPilot"), view_->AdvancedByPilot());
    state.insert(QLatin1String("inDeboardingPhase"), view_->IsInDeboardingPhase());
    state.insert(QLatin1String("cargoAircraft"), view_->IsCargoAircraft());

    state.insert(QLatin1String("fuelProgress"), view_->GetFuelProgress());
    state.insert(QLatin1String("fuelCardLabel"), OperationsViewModel::GetFuelCardLabel());
    state.insert(QLatin1String("fuelProgressText"), view_->GetFuelProgressText());
    state.insert(QLatin1String("loadedFuelLabel"), OperationsViewModel::GetLoadedFuelLabel());
    state.insert(QLatin1String("targetFuelLabel"), OperationsViewModel::GetTargetFuelLabel());
    state.insert(QLatin1String("fuelRateLabel"), OperationsViewModel::GetFuelRateLabel());
    state.insert(QLatin1String("fuelRateText"), view_->GetFuelRateText());
    state.insert(QLatin1String("paxCardLabel"), view_->GetPaxCardLabel());
    state.insert(QLatin1String("paxProgressText"), view_->GetPaxProgressText());
    state.insert(QLatin1String("paxLabel"), OperationsViewModel::GetPaxLabel());
    state.insert(QLatin1String("paxCountText"), view_->GetPaxCountText());
    state.insert(QLatin1String("targetZfwLabel"), OperationsViewModel::GetTargetZfwLabel());
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

    state.insert(QLatin1String("simbriefCardLabel"), OperationsViewModel::GetSimbriefCardLabel());
    state.insert(QLatin1String("simbriefStatusText"), view_->GetSimbriefStatusText());
    state.insert(QLatin1String("plannedFuelLabel"), OperationsViewModel::GetPlannedFuelLabel());
    state.insert(QLatin1String("plannedZfwLabel"), OperationsViewModel::GetPlannedZfwLabel());
    state.insert(QLatin1String("plannedPaxLabel"), OperationsViewModel::GetPlannedPaxLabel());
    state.insert(QLatin1String("plannedPaxText"), view_->GetPlannedPaxText());
    state.insert(QLatin1String("simbriefRefusal"), view_->GetSimbriefRefusal());
    state.insert(QLatin1String("simbriefReady"), view_->IsSimbriefReady());
    state.insert(QLatin1String("simbriefError"), view_->HasSimbriefError());

    state.insert(QLatin1String("gsxProfileConflict"), view_->HasGsxProfileConflict());
    state.insert(QLatin1String("gsxProfileAdvisoryText"), view_->GetGsxProfileAdvisoryText());
    state.insert(QLatin1String("pmdgOptionsAdvisoryText"), OperationsViewModel::GetPmdgOptionsAdvisoryText());
    state.insert(QLatin1String("cargoDoorAdvisoryText"), OperationsViewModel::GetCargoDoorAdvisoryText());
    state.insert(QLatin1String("fuelRequestAdvisoryText"), OperationsViewModel::GetFuelRequestAdvisoryText());
    state.insert(QLatin1String("gsxProfileFixable"), view_->IsGsxProfileFixable());
    state.insert(QLatin1String("pmdgOptionsConflict"), view_->HasPmdgOptionsConflict());
    state.insert(QLatin1String("pmdgOptionsFixable"), view_->IsPmdgOptionsFixable());
    state.insert(QLatin1String("cargoDoorStuck"), view_->IsCargoDoorStuck());
    state.insert(QLatin1String("fuelRequestStalled"), view_->IsFuelRequestStalled());

    state.insert(QLatin1String("canStartFlow"), view_->CanStartFlow());
    state.insert(QLatin1String("canRestartFlow"), view_->CanRestartFlow());
    state.insert(QLatin1String("canStartLoading"), view_->CanStartLoading());
    state.insert(QLatin1String("canReloadSimbrief"), view_->CanReloadSimbrief());
    state.insert(QLatin1String("startFlowLabel"), OperationsViewModel::GetStartFlowLabel());
    state.insert(QLatin1String("startLoadingLabel"), OperationsViewModel::GetStartLoadingLabel());
    state.insert(QLatin1String("restartFlowLabel"), OperationsViewModel::GetRestartFlowLabel());
    state.insert(QLatin1String("confirmRestartLabel"), OperationsViewModel::GetConfirmRestartLabel());
    state.insert(QLatin1String("reloadSimbriefLabel"), OperationsViewModel::GetReloadSimbriefLabel());
    state.insert(QLatin1String("commandErrorLabel"), OperationsViewModel::GetCommandErrorLabel());
    state.insert(QLatin1String("commandError"), view_->GetCommandError());

    return QJsonDocument(state).toJson(QJsonDocument::Compact).toStdString();
}
