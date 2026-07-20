#include "OperationsViewModel.h"

#include <QtCore/QCoreApplication>
#include "../domain/turnaround/TurnaroundPhase.h"
#include "../domain/model/FlightPlan.h"

namespace
{
    QString PhaseLabel(const TurnaroundPhase phase)
    {
        switch (phase)
        {
        case TurnaroundPhase::WaitingSupportedAircraft: return QCoreApplication::translate("Turnaround", "Waiting for sim ready");
        case TurnaroundPhase::WaitingAircraftReady: return QCoreApplication::translate("Turnaround", "Waiting for aircraft ready");
        case TurnaroundPhase::RepositionAircraft: return QCoreApplication::translate("Turnaround", "Repositioning aircraft");
        case TurnaroundPhase::PlaceGroundEquipment: return QCoreApplication::translate("Turnaround", "Placing GPU & chocks");
        case TurnaroundPhase::CallServices: return QCoreApplication::translate("Turnaround", "Starting GSX services");
        case TurnaroundPhase::WaitingPowerOn: return QCoreApplication::translate("Turnaround", "Waiting for power on");
        case TurnaroundPhase::WaitingFlightPlan: return QCoreApplication::translate("Turnaround", "Waiting for flight plan");
        case TurnaroundPhase::RequestFuel: return QCoreApplication::translate("Turnaround", "Requesting fuel");
        case TurnaroundPhase::Refueling: return QCoreApplication::translate("Turnaround", "Refueling");
        case TurnaroundPhase::RequestBoarding: return QCoreApplication::translate("Turnaround", "Requesting boarding");
        case TurnaroundPhase::Boarding: return QCoreApplication::translate("Turnaround", "Boarding");
        case TurnaroundPhase::WaitingReadyToPush: return QCoreApplication::translate("Turnaround", "Preparing for pushback");
        case TurnaroundPhase::WaitCatering: return QCoreApplication::translate("Turnaround", "Waiting for catering");
        case TurnaroundPhase::RemoveGroundEquipment: return QCoreApplication::translate("Turnaround", "Removing GPU & chocks");
        case TurnaroundPhase::RequestPushback: return QCoreApplication::translate("Turnaround", "Requesting pushback");
        case TurnaroundPhase::WaitingPushbackToStart: return QCoreApplication::translate("Turnaround", "Waiting for pushback start");
        case TurnaroundPhase::WaitingForEngines: return QCoreApplication::translate("Turnaround", "Waiting for engines");
        case TurnaroundPhase::WaitingDeparture: return QCoreApplication::translate("Turnaround", "Waiting for departure");
        case TurnaroundPhase::OnFlight: return QCoreApplication::translate("Turnaround", "On flight");
        case TurnaroundPhase::PlaceArrivalGroundEquipment: return QCoreApplication::translate("Turnaround", "Placing GPU & chocks");
        case TurnaroundPhase::WaitingEngineShutdown: return QCoreApplication::translate("Turnaround", "Waiting for engine shutdown");
        case TurnaroundPhase::RequestDeboarding: return QCoreApplication::translate("Turnaround", "Requesting deboarding");
        case TurnaroundPhase::Deboarding: return QCoreApplication::translate("Turnaround", "Deboarding");
        case TurnaroundPhase::CabinServices: return QCoreApplication::translate("Turnaround", "Cabin services");
        case TurnaroundPhase::WaitingNewFlight: return QCoreApplication::translate("Turnaround", "Waiting for new flight");
        default: return QCoreApplication::translate("Turnaround", "Unknown");
        }
    }

    QString PhaseTip(const TurnaroundPhase phase, const bool efbFlightPlan)
    {
        switch (phase)
        {
        case TurnaroundPhase::WaitingAircraftReady:
            return QCoreApplication::translate("Turnaround", "Check that the aircraft engines are shut down.");
        case TurnaroundPhase::WaitingFlightPlan:
            return efbFlightPlan
                       ? QCoreApplication::translate("Turnaround", "Import your SimBrief flight plan on the aircraft EFB.")
                       : QCoreApplication::translate("Turnaround", "Check that SimBrief is loaded in GSX and in this app.");
        case TurnaroundPhase::WaitingPowerOn:
            return QCoreApplication::translate("Turnaround", "Connect the GPU and switch on the batteries so the aircraft has power.");
        case TurnaroundPhase::RequestPushback:
            return QCoreApplication::translate("Turnaround", "Remember to remove additional services (like the GPU).");
        case TurnaroundPhase::WaitingReadyToPush:
            return QCoreApplication::translate("Turnaround", "When you are ready to pushback, turn on the beacon lights.");
        case TurnaroundPhase::WaitingPushbackToStart:
            return QCoreApplication::translate("Turnaround", "Select the final pushback position in the GSX menu.");
        case TurnaroundPhase::WaitingForEngines:
            return QCoreApplication::translate("Turnaround", "Confirm a good engine start with the SmartSwitch.");
        case TurnaroundPhase::WaitingEngineShutdown:
            return QCoreApplication::translate("Turnaround", "Shut down the engines.");
        case TurnaroundPhase::PlaceArrivalGroundEquipment:
            return QCoreApplication::translate("Turnaround", "Set the parking brake.");
        case TurnaroundPhase::RequestDeboarding:
            return QCoreApplication::translate("Turnaround", "Turn off the beacon lights and set the parking brake.");
        case TurnaroundPhase::WaitingNewFlight:
            return QCoreApplication::translate("Turnaround", "Activate the SmartSwitch to start a new flight.");
        default:
            return {};
        }
    }

    QString StartLoadingLabel()
    {
        return QCoreApplication::translate("Turnaround", "Waiting for start loading");
    }

    QString StartLoadingTip()
    {
        return QCoreApplication::translate("Turnaround",
                                           "Press START LOADING or activate the SmartSwitch to begin refueling and boarding.");
    }

    QString FlightPlanStatusLabel(const FlightPlanStatus status)
    {
        switch (status)
        {
        case FlightPlanStatus::Idle: return QCoreApplication::translate("Turnaround", "Inactive");
        case FlightPlanStatus::Fetching: return QCoreApplication::translate("Turnaround", "Fetching");
        case FlightPlanStatus::Ready: return QCoreApplication::translate("Turnaround", "Ready");
        case FlightPlanStatus::Error: return QCoreApplication::translate("Turnaround", "Error");
        }
        return QCoreApplication::translate("Turnaround", "Unknown");
    }
}

OperationsViewModel::OperationsViewModel(IntegratorService* service, QObject* parent)
    : QObject(parent), service_(service), snapshot_(service_->GetSnapshot())
{
    service_->AddObserver(this);
}

OperationsViewModel::~OperationsViewModel()
{
    service_->RemoveObserver(this);
}

bool OperationsViewModel::IsConnected() const
{
    return snapshot_.connected;
}

bool OperationsViewModel::IsSessionActive() const
{
    return snapshot_.sessionActive;
}

bool OperationsViewModel::IsEnabled() const
{
    return snapshot_.automationEnabled;
}

void OperationsViewModel::SetEnabled(const bool enabled)
{
    if (enabled == snapshot_.automationEnabled)
    {
        return;
    }

    SetCommandError(service_->SetAutomationEnabled(enabled));
    Refresh();
}

bool OperationsViewModel::IsGsxAvailable() const
{
    return snapshot_.gsxAvailable;
}

bool OperationsViewModel::IsAircraftSupported() const
{
    return snapshot_.aircraftSupported;
}

QString OperationsViewModel::GetAircraftName() const
{
    return QString::fromStdString(snapshot_.aircraftName);
}

QString OperationsViewModel::GetStateText() const
{
    return IsAwaitingStartLoading() ? StartLoadingLabel() : PhaseLabel(snapshot_.phase);
}

int OperationsViewModel::GetPhase() const
{
    return static_cast<int>(snapshot_.phase);
}

int OperationsViewModel::GetPhaseCount()
{
    return static_cast<int>(TurnaroundPhase::Count);
}

QString OperationsViewModel::GetPhaseTip() const
{
    return IsAwaitingStartLoading() ? StartLoadingTip() : PhaseTip(snapshot_.phase, snapshot_.efbFlightPlan);
}

bool OperationsViewModel::IsAwaitingStartLoading() const
{
    return snapshot_.canStartLoading;
}

int OperationsViewModel::GetDelayTicksRemaining() const
{
    return snapshot_.delayTicksRemaining;
}

QString OperationsViewModel::phaseLabelAt(const int index)
{
    if (index < 0 || index >= static_cast<int>(TurnaroundPhase::Count))
    {
        return {};
    }
    return PhaseLabel(static_cast<TurnaroundPhase>(index));
}

bool OperationsViewModel::IsInDeboardingPhase() const
{
    return snapshot_.phase >= TurnaroundPhase::WaitingEngineShutdown;
}

double OperationsViewModel::GetFuelProgress() const
{
    return snapshot_.fuelProgress;
}

double OperationsViewModel::GetBoardingProgress() const
{
    return snapshot_.boardingProgress;
}

double OperationsViewModel::GetDeboardingProgress() const
{
    return snapshot_.deboardingProgress;
}

double OperationsViewModel::GetPlannedFuelKg() const
{
    return snapshot_.plannedFuelKg;
}

double OperationsViewModel::GetLoadedFuelKg() const
{
    return snapshot_.loadedFuelKg;
}

bool OperationsViewModel::RefuelByGsx() const
{
    return snapshot_.refuelByGsx;
}

bool OperationsViewModel::RefuelBySelf() const
{
    return snapshot_.refuelBySelf;
}

bool OperationsViewModel::HasGsxProfileConflict() const
{
    return snapshot_.gsxProfileConflict;
}

bool OperationsViewModel::IsGsxProfileFixable() const
{
    return snapshot_.gsxProfileFixable;
}

double OperationsViewModel::GetPlannedZfwKg() const
{
    return snapshot_.plannedZfwKg;
}

int OperationsViewModel::GetPlannedPax() const
{
    return snapshot_.plannedPax;
}

int OperationsViewModel::GetBoardedPax() const
{
    return snapshot_.boardedPax;
}

double OperationsViewModel::GetTargetFuelKg() const
{
    return snapshot_.targetFuelKg;
}

double OperationsViewModel::GetTargetZfwKg() const
{
    return snapshot_.targetZfwKg;
}

int OperationsViewModel::GetTargetPax() const
{
    return snapshot_.targetPax;
}

bool OperationsViewModel::IsCargoAircraft() const
{
    return snapshot_.cargoAircraft;
}

QString OperationsViewModel::GetSimbriefStatusText() const
{
    return FlightPlanStatusLabel(snapshot_.flightPlanStatus);
}

bool OperationsViewModel::IsSimbriefReady() const
{
    return snapshot_.flightPlanStatus == FlightPlanStatus::Ready;
}

bool OperationsViewModel::HasSimbriefError() const
{
    return snapshot_.flightPlanStatus == FlightPlanStatus::Error;
}

bool OperationsViewModel::CanToggleAutomation() const
{
    return snapshot_.canToggleAutomation;
}

bool OperationsViewModel::CanStartLoading() const
{
    return snapshot_.canStartLoading;
}

bool OperationsViewModel::CanReloadSimbrief() const
{
    return snapshot_.canReloadSimbrief;
}

QString OperationsViewModel::GetCommandError() const
{
    return commandError_;
}

void OperationsViewModel::startFlow()
{
    SetEnabled(true);
}

void OperationsViewModel::startLoading()
{
    SetCommandError(service_->StartLoading());
    Refresh();
}

void OperationsViewModel::restartFlow()
{
    SetCommandError(service_->RestartFlow());
    Refresh();
}

void OperationsViewModel::reloadSimbrief()
{
    SetCommandError(service_->ReloadSimbrief());
    Refresh();
}

void OperationsViewModel::fixGsxProfile()
{
    SetCommandError(service_->FixGsxProfile());
    Refresh();
}

bool OperationsViewModel::AreDebugToolsAvailable()
{
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
}

void OperationsViewModel::debugSkipPhase(const int delta)
{
#ifndef NDEBUG
    service_->DebugSkipPhase(delta);
    Refresh();
#else
    Q_UNUSED(delta)
#endif
}

void OperationsViewModel::OnIntegratorStateChanged()
{
    Refresh();
}

void OperationsViewModel::RetranslateUi()
{
    emit SnapshotChanged();
}

void OperationsViewModel::Refresh()
{
    const IntegratorSnapshot next = service_->GetSnapshot();
    if (AreEquivalent(snapshot_, next))
    {
        return;
    }

    snapshot_ = next;
    emit SnapshotChanged();
}

void OperationsViewModel::SetCommandError(const CommandResult& result)
{
    const QString message = result.succeeded ? QString() : QString::fromStdString(result.message);
    if (commandError_ == message)
    {
        return;
    }
    commandError_ = message;
    emit CommandErrorChanged();
}
