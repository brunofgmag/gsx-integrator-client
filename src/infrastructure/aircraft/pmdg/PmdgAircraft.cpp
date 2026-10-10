#include "PmdgAircraft.h"

#include <utility>
#include "../DoorReading.h"
#include "../../gsx/GsxLVars.h"
#include "../../pmdg/PmdgDataGateway.h"
#include "../../simvars/SimVars.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/model/FlightPlan.h"

using namespace simvars;

namespace
{
    constexpr auto kSimOnGround = "SIM ON GROUND";

    constexpr int kEngineCount = 2;

    constexpr double kEngineRunningDefault = 1.0;
    constexpr double kEngineCombustionDefault = 0.0;

    constexpr auto kPlanImportedKey = "pmdg.efbPlanImported";
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
                   std::move(spec.smartSwitchPressed)),
      doorAutomationRule_(*variableGateway, *data),
      doorRule_(*data, doors_, doorReconciler_, spec.cargoVariant, spec.mainDeckDoorSlot),
      groundConnectionRule_(*data, groundConn_),
      payloadRule_(*data, payload_),
      rules_{&doorAutomationRule_, &doorRule_, &groundConnectionRule_, &payloadRule_}
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
    QueryTabletState();
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

const std::vector<AircraftRule*>& PmdgAircraft::Rules() const
{
    return rules_;
}

void PmdgAircraft::QueryTabletState()
{
    if (!tablet_->IsAvailable() || ++ticksSinceStateQuery_ < kStateQueryTicks)
    {
        return;
    }

    ticksSinceStateQuery_ = 0;
    stateQuestionSent_ = true;
    tablet_->RequestState();
}

void PmdgAircraft::OnLoadingStarted()
{
    payload_.Reset();
    tablet_->SetEfbPlanImported(false);
    routeImport_.Restart();
}

bool PmdgAircraft::IsReachable() const
{
    return data_->HasData()
        && tablet_->IsAvailable()
        && tablet_->LastWeightEcho().has_value()
        && variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit)
        && variableGateway_->HasReceivedAVar(kSimFuelTotalKg, kKgUnit);
}

void PmdgAircraft::OnTurnaroundStarted()
{
    doorRule_.ForgetMainDeckDoor();
}

void PmdgAircraft::OnTurnaroundResumed(const TurnaroundFacts& facts, const MemoryBag& memory)
{
    doors_.RestoreMemory(memory, facts.gsxRestartedSinceSave);
    doorReconciler_.RestoreMemory(memory);
    doorRule_.RestoreMemory(memory);
    groundConn_.RestoreMemory(memory);
    tablet_->SetEfbPlanImported(memory.Flag(kPlanImportedKey, false));
    payload_.Resume(facts.emptyZfwKg, facts.plannedZfwKg, facts.plannedPassengers);
}

MemoryBag PmdgAircraft::TurnaroundMemory() const
{
    MemoryBag memory;
    doors_.AppendMemory(memory);
    doorReconciler_.AppendMemory(memory);
    doorRule_.AppendMemory(memory);
    groundConn_.AppendMemory(memory);

    if (tablet_->EfbPlanImported() || routeImport_.Seen())
    {
        memory.PutFlag(kPlanImportedKey, true);
    }

    return memory;
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

    return GroundPowerPresent() ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
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
        return doors::kMainDeckDoorMovingLimitTicks;
    }

    if (slot == DoorSlotFor(GsxDoor::FwdCargo) || slot == DoorSlotFor(GsxDoor::AftCargo))
    {
        return doors::kCargoDoorMovingLimitTicks;
    }

    return doors::kPaxDoorMovingLimitTicks;
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

void PmdgAircraft::HoldPassengerDoorsClosed(const bool hold)
{
    doors_.HoldPassengerDoorsClosed(hold);
}
