#include "PmdgAircraft.h"

#include <utility>
#include "DoorReading.h"
#include "../gsx/GsxLVars.h"
#include "../pmdg/PmdgDataGateway.h"
#include "../simvars/SimVars.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/FlightPlan.h"

using namespace simvars;

namespace
{
    constexpr auto kSimOnGround = "SIM ON GROUND";

    constexpr int kEngineCount = 2;

    constexpr double kEngineRunningDefault = 1.0;
    constexpr double kEngineCombustionDefault = 0.0;

    constexpr int kPaxDoorMovingLimitTicks = 15;
    constexpr int kCargoDoorMovingLimitTicks = 60;
    constexpr int kMainDeckDoorMovingLimitTicks = 120;
}

PmdgAircraft::PmdgAircraft(VariableGateway* variableGateway, const AutomationStatus* status,
                           PmdgDataGateway* data, std::unique_ptr<PmdgTabletGateway> tablet,
                           PmdgAircraftSpec spec)
    : variableGateway_(variableGateway),
      status_(status),
      data_(data),
      tablet_(std::move(tablet)),
      cargoVariant_(spec.cargoVariant),
      doorSlots_(spec.doorSlots),
      mainDeckDoorSlot_(spec.mainDeckDoorSlot),
      movingTicks_(static_cast<std::size_t>(spec.doorSlots), 0),
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

void PmdgAircraft::Observe()
{
    doors_.Observe();
    data_->SetInFlight(variableGateway_->GetAVar(kSimOnGround, kBoolUnit, 1.0) <= 0.0);
    data_->Poll();
    tablet_->Poll();
    RefreshDoors();
    AdvanceMovingDoors();

    if (status_->flightPlanStatus == FlightPlanStatus::Ready)
    {
        routeImport_.Observe(PmdgRouteFile::DirectoryFor(GetName()), status_->plannedOrigin,
                             status_->plannedDestination, status_->planGeneratedEpoch);
    }

    if (data_->HasData())
    {
        smartSwitch_.Subscribe();
    }
}

void PmdgAircraft::OnTick()
{
    Observe();

    if (data_->HasData())
    {
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
        SyncMainDeckDoor();
    }

    doorReconciler_.Reconcile();
}

void PmdgAircraft::SyncMainDeckDoor()
{
    const bool loaderPresent = gsx::states::IsLoaderAtDoor(
        doors_.VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0));

    if (loaderPresent && mainDeckTarget_ != MainDeckTarget::Open)
    {
        doorReconciler_.SetSlotDesired(mainDeckDoorSlot_, true);
        mainDeckTarget_ = MainDeckTarget::Open;
    }
    else if (!loaderPresent && mainDeckTarget_ == MainDeckTarget::Open)
    {
        doorReconciler_.SetSlotDesired(mainDeckDoorSlot_, false);
        mainDeckTarget_ = MainDeckTarget::Closed;
    }
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
        mainDeckTarget_ = MainDeckTarget::Closed;
    }

    doorReconciler_.Reconcile();
}

void PmdgAircraft::ClearOwnGroundEquipment()
{
    groundConn_.SetPassengerEntryJetway();
}

DoorStatus PmdgAircraft::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (int slot = 0; slot < doorSlots_; ++slot)
    {
        status = doors::Combine(status, DoorOpenAt(slot));
    }

    return status;
}

bool PmdgAircraft::MainDeckDoorStuck() const
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
        || AnyEngineCombusting(*variableGateway_, kEngineCombustionDefault, kEngineCount);
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
    return !IsEngineRunning() && IsHeldInPlace() && !data_->BeaconOn();
}

bool PmdgAircraft::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || ChocksSet();
}

bool PmdgAircraft::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool PmdgAircraft::IsParkingBrakeSet() const
{
    return data_->ParkingBrakeOn();
}

void PmdgAircraft::AdvanceMovingDoors()
{
    for (int slot = 0; slot < doorSlots_; ++slot)
    {
        int& ticks = movingTicks_[static_cast<std::size_t>(slot)];
        ticks = ObserveDoor(slot) == DoorObservation::Moving ? ticks + 1 : 0;
    }
}

int PmdgAircraft::MovingDoorLimitTicks(const int slot) const
{
    if (cargoVariant_ && slot == mainDeckDoorSlot_)
    {
        return kMainDeckDoorMovingLimitTicks;
    }

    if (slot == DoorSlotFor(GsxDoor::FwdCargo) || slot == DoorSlotFor(GsxDoor::AftCargo))
    {
        return kCargoDoorMovingLimitTicks;
    }

    return kPaxDoorMovingLimitTicks;
}

std::optional<bool> PmdgAircraft::DoorOpenAt(const int slot) const
{
    switch (ObserveDoor(slot))
    {
    case DoorObservation::Open:
        return true;
    case DoorObservation::Closed:
        return false;
    case DoorObservation::Moving:
        return movingTicks_[static_cast<std::size_t>(slot)] >= MovingDoorLimitTicks(slot)
                   ? std::optional{true}
                   : std::nullopt;
    default:
        return std::nullopt;
    }
}

void PmdgAircraft::HoldDoorsClosed(const bool hold)
{
    doors_.HoldClosedForDeparture(hold);
}
