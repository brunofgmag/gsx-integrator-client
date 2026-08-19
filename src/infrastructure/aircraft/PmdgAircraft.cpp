#include "PmdgAircraft.h"

#include <utility>
#include "../gsx/GsxLVars.h"
#include "../pmdg/PmdgDataGateway.h"
#include "../simvars/SimVars.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/FlightPlan.h"

using namespace simvars;

namespace
{
    constexpr auto kSimOnGround = "SIM ON GROUND";

    constexpr double kEngineRunningDefault = 1.0;
    constexpr double kEngineCombustionDefault = 0.0;
}

PmdgAircraft::PmdgAircraft(VariableGateway* variableGateway, const AutomationStatus* status,
                           PmdgDataGateway* data, std::unique_ptr<PmdgTabletGateway> tablet,
                           PmdgAircraftSpec spec)
    : variableGateway_(variableGateway),
      status_(status),
      data_(data),
      tablet_(std::move(tablet)),
      cargoVariant_(spec.cargoVariant),
      mainDeckDoorSlot_(spec.mainDeckDoorSlot),
      doors_(variableGateway),
      doorReconciler_(*this, spec.doorSlots, spec.doorBaseline),
      groundConn_(*this, *tablet_),
      payload_(*tablet_, *variableGateway, status, spec.cargoVariant),
      smartSwitch_(*variableGateway, std::move(spec.smartSwitchLVars),
                   std::move(spec.smartSwitchPressed))
{
}

bool PmdgAircraft::IsCargoVariant() const
{
    return cargoVariant_;
}

void PmdgAircraft::OnTick()
{
    data_->SetInFlight(variableGateway_->GetAVar(kSimOnGround, kBoolUnit, 1.0) <= 0.0);
    data_->Poll();
    tablet_->Poll();
    RefreshDoors();

    if (status_->flightPlanStatus == FlightPlanStatus::Ready)
    {
        routeImport_.Observe(PmdgRouteFile::DirectoryFor(GetName()), status_->plannedOrigin,
                             status_->plannedDestination, status_->planGeneratedEpoch);
    }

    if (data_->HasData())
    {
        smartSwitch_.Subscribe();
        SyncDoors();
        groundConn_.Reconcile();
        payload_.Trim();
    }
}

void PmdgAircraft::SyncDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kAutomationDoors, 1.0) != 0.0)
    {
        variableGateway_->SetLVar(gsx::lvars::kAutomationDoors, 0.0);
    }

    doors_.Sync([this](const GsxDoor door, const bool open) { doorReconciler_.SetDesired(door, open); });

    if (cargoVariant_)
    {
        const bool mainLoaderPresent = gsx::states::IsLoaderAtDoor(
            variableGateway_->GetLVar(gsx::lvars::kBaggageLoaderMainState, 0.0));
        doorReconciler_.SetSlotDesired(mainDeckDoorSlot_, mainLoaderPresent);
    }

    doorReconciler_.Reconcile();
}

void PmdgAircraft::OnLoadingStarted()
{
    payload_.Reset();
}

void PmdgAircraft::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, const bool open) { doorReconciler_.SetDesired(door, open); });

    if (cargoVariant_)
    {
        doorReconciler_.SetSlotDesired(mainDeckDoorSlot_, false);
    }

    doorReconciler_.Reconcile();
}

bool PmdgAircraft::IsMainDeckCargoDoorStuck() const
{
    return cargoVariant_ && doorReconciler_.IsStuck(mainDeckDoorSlot_);
}

bool PmdgAircraft::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready
        && (tablet_->EfbPlanImported() || routeImport_.Seen() || HasVendorFlightPlan());
}

double PmdgAircraft::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double PmdgAircraft::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int PmdgAircraft::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double PmdgAircraft::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double PmdgAircraft::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

void PmdgAircraft::SetCurrentFuelKg(const double fuelKg)
{
    payload_.SetFuelKg(fuelKg);
}

double PmdgAircraft::GetCurrentZfwKg() const
{
    return CurrentZfwKg(*variableGateway_);
}

void PmdgAircraft::SetCurrentZfwKg(const double zfwKg)
{
    payload_.SetZfwKg(zfwKg);
}

bool PmdgAircraft::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool PmdgAircraft::IsPowered() const
{
    return HasAircraftPower()
        || AnyEngineCombusting(*variableGateway_, kEngineCombustionDefault);
}

std::optional<GroundPowerStatus> PmdgAircraft::GetGroundPowerStatus() const
{
    if (!data_->HasData())
    {
        return GroundPowerStatus::Unknown;
    }

    return GroundPowerConnected() ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

bool PmdgAircraft::SetChocks(const bool placed)
{
    groundConn_.SetChocks(placed);

    return true;
}

void PmdgAircraft::SetGroundPower(const bool on)
{
    groundConn_.SetGroundPower(on);
}

bool PmdgAircraft::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && data_->BeaconOn();
}

bool PmdgAircraft::IsReadyToDeboard() const
{
    return !IsEngineRunning() && (IsParkingBrakeSet() || ChocksSet()) && !data_->BeaconOn();
}

bool PmdgAircraft::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault);
}

bool PmdgAircraft::IsParkingBrakeSet() const
{
    return data_->ParkingBrakeOn();
}
