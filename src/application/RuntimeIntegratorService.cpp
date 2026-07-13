#include "RuntimeIntegratorService.h"

#include <algorithm>
#include <QtCore/QCoreApplication>
#include "IntegratorRuntime.h"
#include "../domain/model/AutomationStatus.h"
#include "../domain/model/AutomationSettings.h"
#include "../domain/turnaround/TurnaroundPhase.h"

RuntimeIntegratorService::RuntimeIntegratorService(IntegratorRuntime* runtime, QObject* parent)
    : QObject(parent), runtime_(runtime)
{
    connect(runtime_, &IntegratorRuntime::Updated, this, &RuntimeIntegratorService::NotifyObservers);
}

IntegratorSnapshot RuntimeIntegratorService::GetSnapshot() const
{
    const AutomationStatus& status = runtime_->Status();

    IntegratorSnapshot snapshot;
    snapshot.connected = runtime_->IsConnected();
    snapshot.sessionActive = runtime_->IsSessionActive();
    snapshot.automationEnabled = status.enabled;
    snapshot.gsxAvailable = status.gsxAvailable;
    snapshot.aircraftSupported = status.aircraftSupported;
    snapshot.canToggleAutomation = snapshot.connected;
    snapshot.canStartLoading = snapshot.connected
        && status.enabled
        && runtime_->GetPhase() == TurnaroundPhase::RequestFuel
        && !runtime_->Settings().autoStartLoading
        && !runtime_->IsLoadingConfirmed();
    snapshot.canReloadSimbrief = snapshot.connected
        && snapshot.sessionActive
        && runtime_->Settings().simbriefPilotId > 0
        && runtime_->GetPhase() <= TurnaroundPhase::WaitingFlightPlan;
    snapshot.aircraftName = runtime_->GetAircraftName().toStdString();
    snapshot.refuelByGsx = runtime_->IsAircraftRefuelByGsx();
    snapshot.refuelBySelf = runtime_->IsAircraftRefuelBySelf();
    snapshot.gsxProfileConflict = runtime_->HasGsxProfileConflict();
    snapshot.gsxProfileFixable = runtime_->CanFixGsxProfile();
    snapshot.phase = runtime_->GetPhase();
    snapshot.flightPlanStatus = status.flightPlanStatus;
    snapshot.fuelProgress = status.fuelProgress;
    snapshot.boardingProgress = status.boardingProgress;
    snapshot.deboardingProgress = status.deboardingProgress;
    snapshot.plannedFuelKg = status.plannedFuelKg;
    snapshot.loadedFuelKg = status.loadedFuelKg;
    snapshot.plannedZfwKg = status.plannedZfwKg;
    snapshot.plannedPax = status.plannedPassengers;
    snapshot.boardedPax = status.boardedPassengers;

    return snapshot;
}

CommandResult RuntimeIntegratorService::SetAutomationEnabled(const bool enabled)
{
    if (!runtime_->IsConnected())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Simulator is offline.").toStdString());
    }

    runtime_->SetAutomationEnabled(enabled);

    return CommandResult::Success();
}

CommandResult RuntimeIntegratorService::StartLoading()
{
    if (!runtime_->IsConnected())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Simulator is offline.").toStdString());
    }

    if (runtime_->GetPhase() != TurnaroundPhase::RequestFuel)
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "The turnaround is not waiting to start loading.").toStdString());
    }

    runtime_->ConfirmLoading();

    return CommandResult::Success();
}

CommandResult RuntimeIntegratorService::RestartFlow()
{
    if (!runtime_->IsConnected())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Simulator is offline.").toStdString());
    }

    runtime_->RestartFlow();

    return CommandResult::Success();
}

CommandResult RuntimeIntegratorService::ReloadSimbrief()
{
    const IntegratorSnapshot snapshot = GetSnapshot();
    if (!snapshot.connected)
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Simulator is offline.").toStdString());
    }
    if (!snapshot.sessionActive)
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Wait for an active flight session.").toStdString());
    }
    if (runtime_->GetPhase() > TurnaroundPhase::WaitingFlightPlan)
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "The flight plan can no longer be reloaded during the turnaround.").toStdString());
    }
    if (runtime_->Settings().simbriefPilotId <= 0)
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Configure a valid SimBrief Pilot ID first.").toStdString());
    }
    if (!runtime_->ReloadSimbrief())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Could not start the SimBrief request.").toStdString());
    }

    return CommandResult::Success();
}

CommandResult RuntimeIntegratorService::FixGsxProfile()
{
    if (!runtime_->HasGsxProfileConflict())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "The GSX profile does not need fixing.").toStdString());
    }

    if (!runtime_->FixGsxProfile())
    {
        return CommandResult::Failure(
            QCoreApplication::translate("Integrator", "Could not update the GSX aircraft profile.").toStdString());
    }

    return CommandResult::Success();
}

void RuntimeIntegratorService::ApplySettings(const AppSettings& settings)
{
    AutomationSettings automationSettings;
    automationSettings.simbriefPilotId = settings.simbriefPilotId;
    automationSettings.fuelRateKgs = settings.fuelRateKgs;
    automationSettings.autoSelectGsxChoice = settings.autoSelectGsxChoice;
    automationSettings.autoStartFlow = settings.autoStartFlow;
    automationSettings.autoStartLoading = settings.autoStartLoading;
    automationSettings.skipReposition = settings.skipReposition;

    runtime_->ApplySettings(automationSettings);
}

void RuntimeIntegratorService::AddObserver(IntegratorServiceObserver* observer)
{
    if (observer == nullptr || std::ranges::find(observers_, observer) != observers_.end())
    {
        return;
    }

    observers_.push_back(observer);
}

void RuntimeIntegratorService::RemoveObserver(IntegratorServiceObserver* observer)
{
    observers_.erase(std::ranges::remove(observers_, observer).begin(), observers_.end());
}

void RuntimeIntegratorService::NotifyObservers() const
{
    const auto observers = observers_;
    for (IntegratorServiceObserver* observer : observers)
    {
        if (observer != nullptr)
        {
            observer->OnIntegratorStateChanged();
        }
    }
}
