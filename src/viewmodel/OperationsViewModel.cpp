#include "OperationsViewModel.h"

#include <cmath>
#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>
#include "../domain/turnaround/PilotTouch.h"
#include "../domain/turnaround/TurnaroundPhase.h"
#include "../domain/model/FlightPlan.h"
#include "../domain/support/Weight.h"

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
        case TurnaroundPhase::CallCatering: return QCoreApplication::translate("Turnaround", "Requesting catering");
        case TurnaroundPhase::WaitingFlightPlan: return QCoreApplication::translate("Turnaround", "Waiting for flight plan");
        case TurnaroundPhase::RequestFuel: return QCoreApplication::translate("Turnaround", "Requesting fuel");
        case TurnaroundPhase::Refueling: return QCoreApplication::translate("Turnaround", "Refueling");
        case TurnaroundPhase::RequestBoarding: return QCoreApplication::translate("Turnaround", "Requesting boarding");
        case TurnaroundPhase::Boarding: return QCoreApplication::translate("Turnaround", "Boarding");
        case TurnaroundPhase::WaitingReadyToPush: return QCoreApplication::translate("Turnaround", "Waiting for beacon & brake");
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
            return QCoreApplication::translate("Turnaround", "Turn on the beacon lights and set the parking brake.");
        case TurnaroundPhase::WaitingPushbackToStart:
            return QCoreApplication::translate("Turnaround", "Select the final pushback position in the GSX menu.");
        case TurnaroundPhase::WaitingForEngines:
            return QCoreApplication::translate("Turnaround", "Confirm a good engine start with the SmartSwitch.");
        case TurnaroundPhase::WaitingEngineShutdown:
            return QCoreApplication::translate("Turnaround", "Shut down the engines.");
        case TurnaroundPhase::RemoveGroundEquipment:
        case TurnaroundPhase::PlaceArrivalGroundEquipment:
            return QCoreApplication::translate("Turnaround", "Remember to set the Parking Brake.");
        case TurnaroundPhase::RequestDeboarding:
            return QCoreApplication::translate("Turnaround", "Turn off the beacon lights and set the parking brake.");
        case TurnaroundPhase::WaitingNewFlight:
            return QCoreApplication::translate("Turnaround", "Activate the SmartSwitch to start a new flight.");
        default:
            return {};
        }
    }

    QString PilotTouchLabel(const TurnaroundPhase phase)
    {
        switch (phase)
        {
        case TurnaroundPhase::WaitingReadyToPush:
            return QCoreApplication::translate("OperationsScreen", "Unlock Pushback");
        case TurnaroundPhase::WaitingForEngines:
            return QCoreApplication::translate("OperationsScreen", "Confirm Engine Start");
        case TurnaroundPhase::WaitingNewFlight:
            return QCoreApplication::translate("OperationsScreen", "Start New Flight");
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

    QString PercentText(const double progress)
    {
        return QString::number(qRound(progress)) + QStringLiteral("%");
    }

    QString ConnectionLabel(const bool connected)
    {
        return connected
                   ? QCoreApplication::translate("OperationsScreen", "Connected")
                   : QCoreApplication::translate("OperationsScreen", "Offline");
    }

    QString AutomationModeLabel(const bool automatic)
    {
        return automatic
                   ? QCoreApplication::translate("OperationsScreen", "Auto")
                   : QCoreApplication::translate("OperationsScreen", "Manual");
    }

    QString AutomationRunningLabel(const bool running)
    {
        return running
                   ? QCoreApplication::translate("OperationsScreen", "On")
                   : QCoreApplication::translate("OperationsScreen", "Off");
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

OperationsViewModel::OperationsViewModel(IntegratorService* service, const OperationsDisplaySettings* display,
                                         QObject* parent)
    : QObject(parent), service_(service), display_(display), snapshot_(service_->GetSnapshot())
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

QString OperationsViewModel::WeightText(const double kilograms) const
{
    const bool lb = display_->GetWeightIsLb();
    const double shown = lb ? weight::KgToLb(kilograms) : kilograms;

    return QLocale().toString(qRound64(shown))
        + QStringLiteral(" ")
        + (lb ? QCoreApplication::translate("OperationsScreen", "lb")
              : QCoreApplication::translate("OperationsScreen", "kg"));
}

QString OperationsViewModel::GetPlannedFuelText() const
{
    return WeightText(GetPlannedFuelKg());
}

QString OperationsViewModel::GetLoadedFuelText() const
{
    return WeightText(GetLoadedFuelKg());
}

QString OperationsViewModel::GetTargetFuelText() const
{
    return WeightText(GetTargetFuelKg());
}

QString OperationsViewModel::GetTargetZfwText() const
{
    return WeightText(GetTargetZfwKg());
}

QString OperationsViewModel::GetPlannedZfwText() const
{
    return WeightText(GetPlannedZfwKg());
}

QString OperationsViewModel::GetAircraftNameText() const
{
    return IsAircraftSupported()
        ? GetAircraftName()
        : QCoreApplication::translate("OperationsScreen", "Standby");
}

QString OperationsViewModel::GetSimLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Sim");
}

QString OperationsViewModel::GetSimStatusText() const
{
    return ConnectionLabel(IsConnected());
}

QString OperationsViewModel::GetGsxLabel()
{
    return QCoreApplication::translate("OperationsScreen", "GSX Pro");
}

QString OperationsViewModel::GetGsxStatusText() const
{
    return ConnectionLabel(IsGsxAvailable());
}

QString OperationsViewModel::GetAircraftLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Aircraft");
}

QString OperationsViewModel::GetTurnaroundModeLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Turnaround");
}

QString OperationsViewModel::GetTurnaroundModeText() const
{
    return QCoreApplication::translate("OperationsScreen", "%1 · %2")
        .arg(AutomationModeLabel(display_->GetAutoStartFlow()),
             AutomationRunningLabel(snapshot_.automationEnabled));
}

QString OperationsViewModel::GetLoadingModeLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Loading");
}

QString OperationsViewModel::GetLoadingModeText() const
{
    return AutomationModeLabel(display_->GetAutoStartLoading());
}

bool OperationsViewModel::AutoStartsFlow() const
{
    return display_->GetAutoStartFlow();
}

bool OperationsViewModel::IsLoadingRunning() const
{
    return snapshot_.phase == TurnaroundPhase::Refueling
        || snapshot_.phase == TurnaroundPhase::Boarding
        || snapshot_.phase == TurnaroundPhase::Deboarding;
}

bool OperationsViewModel::AutoStartsLoading() const
{
    return display_->GetAutoStartLoading();
}

QString OperationsViewModel::GetTurnaroundStateLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Turnaround state");
}

QString OperationsViewModel::GetPhaseCounterText() const
{
    return QString::number(GetPhase() + 1)
        + QStringLiteral("/")
        + QString::number(GetPhaseCount());
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

QString OperationsViewModel::GetNextPhaseText() const
{
    const int next = GetPhase() + 1;
    const QString label = next < GetPhaseCount()
        ? PhaseLabel(static_cast<TurnaroundPhase>(next))
        : QCoreApplication::translate("OperationsScreen", "New session");

    return QCoreApplication::translate("OperationsScreen", "Next")
        + QStringLiteral(" ▸ ")
        + label;
}

QString OperationsViewModel::GetHoldCountdownText() const
{
    const int remaining = GetDelayTicksRemaining();
    if (remaining <= 0)
    {
        return {};
    }

    return QCoreApplication::translate("OperationsScreen", "Next state in %1s")
        .arg(remaining);
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

bool OperationsViewModel::IsInDeboardingPhase() const
{
    return snapshot_.phase >= TurnaroundPhase::WaitingEngineShutdown;
}

QString OperationsViewModel::GetFuelCardLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Fuel");
}

QString OperationsViewModel::GetFuelProgressText() const
{
    return PercentText(GetFuelProgress());
}

QString OperationsViewModel::GetLoadedFuelLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Loaded");
}

QString OperationsViewModel::GetTargetFuelLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Planned");
}

QString OperationsViewModel::GetFuelRateLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Rate");
}

QString OperationsViewModel::GetFuelRateText() const
{
    if (RefuelByGsx())
    {
        return QCoreApplication::translate("OperationsScreen", "Auto");
    }

    if (RefuelBySelf())
    {
        return QStringLiteral("GSX");
    }

    return display_->GetFuelRateText()
        + QStringLiteral(" ")
        + display_->GetFuelRateUnitText();
}

QString OperationsViewModel::GetPaxCardLabel() const
{
    return IsInDeboardingPhase()
               ? QCoreApplication::translate("OperationsScreen", "Deboarding")
               : QCoreApplication::translate("OperationsScreen", "Boarding");
}

QString OperationsViewModel::GetPaxProgressText() const
{
    return PercentText(IsInDeboardingPhase() ? GetDeboardingProgress() : GetBoardingProgress());
}

QString OperationsViewModel::GetPaxLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Pax");
}

QString OperationsViewModel::GetPaxCountText() const
{
    return QString::number(IsInDeboardingPhase() ? GetDeboardedPax() : GetBoardedPax())
        + QStringLiteral(" / ")
        + QString::number(GetTargetPax());
}

QString OperationsViewModel::GetTargetZfwLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Planned ZFW");
}

QString OperationsViewModel::GetSimbriefCardLabel()
{
    return QCoreApplication::translate("OperationsScreen", "SimBrief OFP");
}

QString OperationsViewModel::GetPlannedFuelLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Fuel");
}

QString OperationsViewModel::GetPlannedZfwLabel()
{
    return QCoreApplication::translate("OperationsScreen", "ZFW");
}

QString OperationsViewModel::GetPlannedPaxLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Pax");
}

QString OperationsViewModel::GetPlannedPaxText() const
{
    return QString::number(GetPlannedPax());
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

QString OperationsViewModel::GetGsxProfileAdvisoryText() const
{
    return IsGsxProfileFixable()
               ? QCoreApplication::translate("OperationsScreen",
                                             "The GSX profile for this aircraft does not set 'refueling = 0', so the fuel truck never connects the hose. Apply the fix, then restart GSX or reload the flight.")
               : QCoreApplication::translate("OperationsScreen",
                                             "No GSX profile with 'refueling = 0' was found for this aircraft. Install an aircraft profile and set 'refueling = 0' in its gsx.cfg.");
}

QString OperationsViewModel::GetGsxProfileActionLabel() const
{
    return IsGsxProfileFixable()
               ? QCoreApplication::translate("OperationsScreen", "Fix profile")
               : QString();
}

QString OperationsViewModel::GetPmdgOptionsAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "The PMDG options file does not enable the SDK data broadcast, so the client cannot read this aircraft. Apply the fix, then reload the flight.");
}

QString OperationsViewModel::GetPmdgOptionsActionLabel() const
{
    return IsPmdgOptionsFixable()
               ? QCoreApplication::translate("OperationsScreen", "Enable broadcast")
               : QString();
}

QString OperationsViewModel::GetCargoDoorAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "A GSX loader is waiting for the main deck cargo door. That door runs on hydraulics, so switch the ELEC 2 pump on in the overhead.");
}

QString OperationsViewModel::GetFuelRequestAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "GSX took the refuelling request but the truck has not arrived. Check the GSX menu, or another service may be holding it.");
}

QString OperationsViewModel::GetFuelPlanAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "The flight plan asks for more fuel than this airframe can hold. The tanks will be filled to capacity and no further.");
}

QString OperationsViewModel::GetServicesAdvisoryText() const
{
    return QCoreApplication::translate("OperationsScreen", "GSX has not answered the request yet and nothing is moving. The client moves on in %1 s.")
        .arg(snapshot_.servicesWaitSeconds);
}

QString OperationsViewModel::GetOpenDoorAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "A door is open. Close it, or use the SmartSwitch to unlock the pushback.");
}

QString OperationsViewModel::GetServiceInterruptedAdvisoryText()
{
    return QCoreApplication::translate("OperationsScreen",
                                       "GSX dropped a service it had already started. Ask for it again from the GSX menu; the client will pick the turnaround back up.");
}

QString OperationsViewModel::GetCommandErrorLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Error");
}

bool OperationsViewModel::HasGsxProfileConflict() const
{
    return snapshot_.gsxProfileConflict;
}

bool OperationsViewModel::IsGsxProfileFixable() const
{
    return snapshot_.gsxProfileFixable;
}

bool OperationsViewModel::HasPmdgOptionsConflict() const
{
    return snapshot_.pmdgOptionsConflict;
}

bool OperationsViewModel::IsCargoDoorStuck() const
{
    return snapshot_.cargoDoorStuck;
}

bool OperationsViewModel::IsFuelRequestStalled() const
{
    return snapshot_.fuelRequestStalled;
}

bool OperationsViewModel::IsFuelPlanOverCapacity() const
{
    return snapshot_.fuelPlanOverCapacity;
}

bool OperationsViewModel::AreServicesStalled() const
{
    return snapshot_.servicesStalled;
}

int OperationsViewModel::GetServicesWaitSeconds() const
{
    return snapshot_.servicesWaitSeconds;
}

bool OperationsViewModel::IsServiceInterrupted() const
{
    return snapshot_.serviceInterrupted;
}

bool OperationsViewModel::AreDoorsHoldingPushback() const
{
    return snapshot_.doorsHoldingPushback;
}

bool OperationsViewModel::IsPmdgOptionsFixable() const
{
    return snapshot_.pmdgOptionsFixable;
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

int OperationsViewModel::GetDeboardedPax() const
{
    return static_cast<int>(std::lround(snapshot_.deboardingProgress / 100.0 * snapshot_.targetPax));
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

int OperationsViewModel::GetAutoWeightUnit() const
{
    return snapshot_.autoWeightUnit;
}

bool OperationsViewModel::IsCargoAircraft() const
{
    return snapshot_.cargoAircraft;
}

QString OperationsViewModel::GetSimbriefStatusText() const
{
    if (!snapshot_.simbriefRefusal.empty())
    {
        return QCoreApplication::translate("Turnaround", "Refused");
    }

    return FlightPlanStatusLabel(snapshot_.flightPlanStatus);
}

bool OperationsViewModel::IsSimbriefReady() const
{
    return snapshot_.simbriefRefusal.empty() && snapshot_.flightPlanStatus == FlightPlanStatus::Ready;
}

bool OperationsViewModel::HasSimbriefError() const
{
    return !snapshot_.simbriefRefusal.empty() || snapshot_.flightPlanStatus == FlightPlanStatus::Error;
}

QString OperationsViewModel::GetSimbriefRefusal() const
{
    return QString::fromStdString(snapshot_.simbriefRefusal);
}

QString OperationsViewModel::GetSimbriefFailureText() const
{
    if (!snapshot_.simbriefRefusal.empty())
    {
        return QString::fromStdString(snapshot_.simbriefRefusal);
    }

    switch (snapshot_.flightPlanFailure)
    {
    case FlightPlanFailure::NotSent:
        return QCoreApplication::translate("Turnaround", "The SimBrief request was never sent");
    case FlightPlanFailure::Http:
        return QCoreApplication::translate("Turnaround", "SimBrief answered HTTP %1")
            .arg(snapshot_.flightPlanHttpStatus);
    case FlightPlanFailure::Parse:
        return QCoreApplication::translate("Turnaround", "SimBrief answered a flight plan the client could not read");
    case FlightPlanFailure::None:
        break;
    }

    return {};
}

QString OperationsViewModel::GetStartFlowLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Start Flow");
}

QString OperationsViewModel::GetStartLoadingLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Start Loading");
}

QString OperationsViewModel::GetRestartFlowLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Restart Flow");
}

QString OperationsViewModel::GetConfirmRestartLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Confirm restart");
}

QString OperationsViewModel::GetReloadSimbriefLabel()
{
    return QCoreApplication::translate("OperationsScreen", "Reload SimBrief");
}

bool OperationsViewModel::CanStartFlow() const
{
    return snapshot_.canToggleAutomation && !snapshot_.automationEnabled && !display_->GetAutoStartFlow();
}

bool OperationsViewModel::CanRestartFlow() const
{
    return snapshot_.connected && snapshot_.automationEnabled;
}

bool OperationsViewModel::CanStartLoading() const
{
    return snapshot_.canStartLoading;
}

bool OperationsViewModel::CanReloadSimbrief() const
{
    return snapshot_.canReloadSimbrief;
}

QString OperationsViewModel::GetPilotTouchLabel() const
{
    return PilotTouchLabel(snapshot_.phase);
}

bool OperationsViewModel::CanPilotTouch() const
{
    return snapshot_.connected && snapshot_.automationEnabled && PilotTouch::Accepts(snapshot_.phase);
}

QString OperationsViewModel::GetCommandError() const
{
    return commandError_;
}

void OperationsViewModel::startFlow()
{
    SetEnabled(true);
}

void OperationsViewModel::AcceptPilotTouch(const TurnaroundPhase stamped)
{
    SetCommandError(service_->AcceptPilotTouch(stamped));
    Refresh();
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

void OperationsViewModel::fixPmdgOptions()
{
    SetCommandError(service_->FixPmdgOptions());
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

void OperationsViewModel::RefreshDisplayText()
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
