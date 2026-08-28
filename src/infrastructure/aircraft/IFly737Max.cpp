#include "IFly737Max.h"

#include "../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "DoorReading.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/ports/GsxGateway.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kSimAvionicsBusVoltage = "ELECTRICAL AVIONICS BUS VOLTAGE";
    constexpr auto kVoltsUnit = "Volts";

    constexpr auto kSmartSwitch = "VC_ACP_1_Push_to_Talk_SW_VAL";
    constexpr double kSmartSwitchNeutral = 10.0;

    constexpr int kEngineCount = 2;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kParkingBrakeLVar = "VC_Parking_Brake_SW_VAL";
    constexpr auto kChocksLVar = "iFly_NLG_Chock_Display_VAL";

    constexpr auto kSimPayloadStationPrefix = "PAYLOAD STATION WEIGHT:";
    constexpr std::array kStationDefaultLoadsLbs =
        {2100.0, 5250.0, 1050.0, 2975.0, 3150.0, 3500.0, 2275.0, 5752.0, 8018.0};

    constexpr auto kFwdCargoAnimLVar = "Animation_FWD_Cargo_VAL";
    constexpr auto kAftCargoAnimLVar = "Animation_AFT_Cargo_VAL";
    constexpr double kCargoDoorOpenThreshold = 90.0;
    constexpr double kCargoDoorFullyOpen = 100.0;
    constexpr double kJetwayAtAircraft = 4.0;
    constexpr std::array kCargoDoorAnimLVars = {kFwdCargoAnimLVar, kAftCargoAnimLVar};

    constexpr std::array kPaxDoorAnimLVars = {
        "ANIMATION_FWD_ENTRY_VAL", "ANIMATION_FWD_SERVICE_VAL",
        "ANIMATION_AFT_ENTRY_VAL", "ANIMATION_AFT_SERVICE_VAL",
        "ANIMATION_L_FWD_OVERWING_VAL", "ANIMATION_R_FWD_OVERWING_VAL",
        "ANIMATION_L_AFT_OVERWING_VAL", "ANIMATION_R_AFT_OVERWING_VAL"
    };
    constexpr int kPulseSettleTicks = 15;
    constexpr int kMaxDoorPulseAttempts = 5;
    constexpr int kAircraftOpensItselfTicks = 10;

    bool IsState(const double lvarValue, const GsxStateStatus state)
    {
        return lvarValue == static_cast<double>(state);
    }
}

IFly737Max::IFly737Max(VariableGateway* variableGateway, const AutomationStatus* status)
    : variableGateway_(variableGateway), status_(status),
      smartSwitch_(*variableGateway, {kSmartSwitch},
                   [](const double min, const double max)
                   {
                       return min < kSmartSwitchNeutral || max > kSmartSwitchNeutral;
                   }),
      fwdCargoDoor_{
          "FWD", kFwdCargoAnimLVar, gsx::lvars::kAircraftCargo1Toggle,
          gsx::lvars::kBaggageLoaderFrontState
      },
      aftCargoDoor_{
          "AFT", kAftCargoAnimLVar, gsx::lvars::kAircraftCargo2Toggle,
          gsx::lvars::kBaggageLoaderRearState
      },
      paxDoors_{
          CargoDoorCloser{
              "1L", "ANIMATION_FWD_ENTRY_VAL", gsx::lvars::kAircraftExit1Toggle,
              gsx::lvars::kPassengerStairsFrontState, DoorKind::JetwayOrStairs
          },
          CargoDoorCloser{
              "2L", "ANIMATION_AFT_ENTRY_VAL", gsx::lvars::kAircraftExit4Toggle,
              gsx::lvars::kPassengerStairsRearState, DoorKind::Stairs
          },
          CargoDoorCloser{
              "1R", "ANIMATION_FWD_SERVICE_VAL", gsx::lvars::kAircraftService1Toggle,
              gsx::lvars::kCateringFrontState, DoorKind::Catering
          },
          CargoDoorCloser{
              "2R", "ANIMATION_AFT_SERVICE_VAL", gsx::lvars::kAircraftService2Toggle,
              gsx::lvars::kCateringRearState, DoorKind::Catering
          }
      }
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: iFly 737 MAX 8");
}

void IFly737Max::OnTick()
{
    DriveDoors();
}

void IFly737Max::OnSlowTick()
{
    planImport_.Observe(IFlyPlanFile::DirectoryFor(), status_->planGeneratedEpoch);
}

std::array<IFly737Max::CargoDoorCloser*, 6> IFly737Max::AllDoors()
{
    return {
        &fwdCargoDoor_, &aftCargoDoor_,
        &paxDoors_[0], &paxDoors_[1], &paxDoors_[2], &paxDoors_[3]
    };
}

void IFly737Max::CloseAllDoors()
{
    for (CargoDoorCloser& door : paxDoors_)
    {
        ResetDoorTracking(door);
        door.closeRequested = true;
    }
}

void IFly737Max::HoldDoorsClosed(const bool hold)
{
    heldForDeparture_ = hold;

    if (!hold)
    {
        return;
    }

    for (CargoDoorCloser& door : paxDoors_)
    {
        ResetDoorTracking(door);
    }
}

bool IFly737Max::WantsOpen(const CargoDoorCloser& door) const
{
    const double equipment = variableGateway_->GetLVar(door.loaderLVar, 0.0);

    switch (door.kind)
    {
    case DoorKind::JetwayOrStairs:
        return variableGateway_->GetLVar(gsx::lvars::kJetway, 0.0) >= kJetwayAtAircraft
            || gsx::states::AreStairsArriving(equipment);
    case DoorKind::Stairs:
        return gsx::states::AreStairsArriving(equipment);
    case DoorKind::Catering:
        return gsx::states::IsCateringArriving(equipment);
    default:
        return gsx::states::IsLoaderAtDoor(equipment);
    }
}

void IFly737Max::DriveDoors()
{
    const CargoCycle cycle = CurrentCargoCycle();

    if (cycle != CargoCycle::None && cycle != armedCycle_)
    {
        ArmCargoDoorCloser(cycle);
    }

    LatchCycleCompletion();

    for (CargoDoorCloser* door : AllDoors())
    {
        TrackDoor(*door);
    }

    const bool busy = AdvanceDoorPulse();

    if (!busy && cycle == CargoCycle::None && !HasPendingCargoDoorWork())
    {
        DisarmCargoDoorCloser();
    }
}

void IFly737Max::LatchCycleCompletion()
{
    if (armedCycle_ == CargoCycle::Boarding && IsStateCompleted(gsx::lvars::kBoardingState))
    {
        boardingCompleteSeen_ = true;
    }

    if (armedCycle_ == CargoCycle::Deboarding && IsStateCompleted(gsx::lvars::kDeboardingState))
    {
        deboardingCompleteSeen_ = true;
    }
}

void IFly737Max::TrackDoor(CargoDoorCloser& door) const
{
    TrackDoorTravel(door);

    if (WantsOpen(door))
    {
        ++door.wantsOpenTicks;
    }
    else
    {
        door.wantsOpenTicks = 0;
        door.openAttempts = 0;
    }

    if (armedCycle_ == CargoCycle::Deboarding && door.kind == DoorKind::Cargo)
    {
        TrackBaggageLoader(door);
    }
}

IFly737Max::CargoCycle IFly737Max::CurrentCargoCycle() const
{
    if (IsStateActive(gsx::lvars::kBoardingState))
    {
        return CargoCycle::Boarding;
    }

    if (IsStateActive(gsx::lvars::kDeboardingState))
    {
        return CargoCycle::Deboarding;
    }

    return CargoCycle::None;
}

bool IFly737Max::IsStateActive(const char* stateLVar) const
{
    return IsState(variableGateway_->GetLVar(stateLVar, 0.0), GsxStateStatus::Active);
}

bool IFly737Max::IsStateCompleted(const char* stateLVar) const
{
    return IsState(variableGateway_->GetLVar(stateLVar, 0.0), GsxStateStatus::Completed);
}

void IFly737Max::ArmCargoDoorCloser(const CargoCycle cycle)
{
    armedCycle_ = cycle;
    boardingCompleteSeen_ = false;
    deboardingCompleteSeen_ = false;
    ResetDoorTracking(fwdCargoDoor_);
    ResetDoorTracking(aftCargoDoor_);
}

void IFly737Max::ResetDoorTracking(CargoDoorCloser& door)
{
    door.servedSeen = false;
    door.loaderDone = false;
    door.attempts = 0;
}

bool IFly737Max::AdvanceDoorPulse()
{
    bool busy = false;

    for (CargoDoorCloser* door : AllDoors())
    {
        busy = AdvanceDoorPulse(*door) || busy;
    }

    return busy;
}

bool IFly737Max::AdvanceDoorPulse(CargoDoorCloser& door)
{
    if (door.pulseHigh)
    {
        variableGateway_->SetLVar(door.toggleLVar, 0.0);
        door.pulseHigh = false;
        door.settleTicks = kPulseSettleTicks;

        return true;
    }

    if (door.settleTicks > 0)
    {
        --door.settleTicks;

        return true;
    }

    if (IsDoorOpenable(door))
    {
        return PulseDoor(door, door.openAttempts, "reopening");
    }

    if (IsDoorCloseable(door))
    {
        return PulseDoor(door, door.attempts, "closing");
    }

    return false;
}

bool IFly737Max::PulseDoor(CargoDoorCloser& door, int& attempts, const char* verb)
{
    ++attempts;
    variableGateway_->SetLVar(door.toggleLVar, 1.0);
    door.pulseHigh = true;

    LOG_INFO("iFly: %s %s door (attempt %d/%d)",
             verb, door.doorName, attempts, kMaxDoorPulseAttempts);

    return true;
}

bool IFly737Max::HasPendingCargoDoorWork() const
{
    return IsBaggageLoaderPresent(fwdCargoDoor_.loaderLVar)
        || IsBaggageLoaderPresent(aftCargoDoor_.loaderLVar)
        || IsDoorClosePending(fwdCargoDoor_)
        || IsDoorClosePending(aftCargoDoor_);
}

void IFly737Max::TrackDoorTravel(CargoDoorCloser& door) const
{
    const double anim = variableGateway_->GetLVar(door.animLVar, 0.0);

    door.moving = door.lastAnim >= 0.0 && anim != door.lastAnim;
    door.lastAnim = anim;
}

void IFly737Max::TrackBaggageLoader(CargoDoorCloser& door) const
{
    const double loaderState = variableGateway_->GetLVar(door.loaderLVar, 0.0);

    if (loaderState == gsx::states::kLoaderUnloading || loaderState == gsx::states::kLoaderLoading)
    {
        door.servedSeen = true;
    }

    if (gsx::states::IsLoaderAtDoor(loaderState) || loaderState == gsx::states::kLoaderRetracting)
    {
        door.loaderDone = false;
        door.attempts = 0;

        return;
    }

    if (door.servedSeen)
    {
        door.loaderDone = true;
    }
}

bool IFly737Max::IsBaggageLoaderPresent(const char* loaderLVar) const
{
    return variableGateway_->HasReceivedLVar(loaderLVar)
        && gsx::states::IsLoaderPresent(variableGateway_->GetLVar(loaderLVar, 0.0));
}

bool IFly737Max::IsLoaderAtDoorNow(const CargoDoorCloser& door) const
{
    return door.kind == DoorKind::Cargo && WantsOpen(door);
}


bool IFly737Max::IsDoorReleased(const CargoDoorCloser& door) const
{
    if (door.kind != DoorKind::Cargo)
    {
        return heldForDeparture_ || (door.closeRequested && !WantsOpen(door));
    }

    if (armedCycle_ == CargoCycle::Boarding)
    {
        return boardingCompleteSeen_ && !IsLoaderAtDoorNow(door);
    }

    return (door.loaderDone || (deboardingCompleteSeen_ && door.servedSeen))
        && !IsLoaderAtDoorNow(door);
}

bool IFly737Max::IsDoorCloseable(const CargoDoorCloser& door) const
{
    return IsDoorReleased(door)
        && !door.moving
        && door.attempts < kMaxDoorPulseAttempts
        && variableGateway_->GetLVar(door.animLVar, 0.0) > kCargoDoorOpenThreshold;
}

bool IFly737Max::IsDoorOpenable(const CargoDoorCloser& door) const
{
    return !heldForDeparture_
        && door.wantsOpenTicks >= kAircraftOpensItselfTicks
        && !door.moving
        && door.openAttempts < kMaxDoorPulseAttempts
        && variableGateway_->HasReceivedLVar(door.animLVar)
        && variableGateway_->GetLVar(door.animLVar, kCargoDoorFullyOpen) <= kCargoDoorOpenThreshold;
}

bool IFly737Max::IsDoorClosePending(const CargoDoorCloser& door) const
{
    if (armedCycle_ == CargoCycle::Boarding)
    {
        return !boardingCompleteSeen_ || IsDoorCloseable(door);
    }

    return (door.servedSeen && !door.loaderDone) || IsDoorCloseable(door);
}

void IFly737Max::DisarmCargoDoorCloser()
{
    armedCycle_ = CargoCycle::None;
}

bool IFly737Max::IsCargoVariant() const
{
    return false;
}

bool IFly737Max::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready
        && (planImport_.Seen() || planImport_.Blind());
}

double IFly737Max::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double IFly737Max::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int IFly737Max::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double IFly737Max::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double IFly737Max::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double IFly737Max::GetCurrentZfwKg() const
{
    return CurrentZfwKg(*variableGateway_);
}

void IFly737Max::SetCurrentZfwKg(const double zfwKg)
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit) || zfwKg == lastZfwKg_)
    {
        return;
    }

    lastZfwKg_ = zfwKg;

    const double payloadKg = std::max(zfwKg - GetEmptyZfwKg(), 0.0);

    double totalDefaultLbs = 0.0;
    for (const double stationLbs : kStationDefaultLoadsLbs)
    {
        totalDefaultLbs += stationLbs;
    }

    for (std::size_t i = 0; i < kStationDefaultLoadsLbs.size(); ++i)
    {
        variableGateway_->SetAVar(kSimPayloadStationPrefix + std::to_string(i + 1), kKgUnit,
                                  payloadKg * kStationDefaultLoadsLbs[i] / totalDefaultLbs);
    }
}

bool IFly737Max::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

DoorStatus IFly737Max::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* animLVar : kCargoDoorAnimLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, animLVar));
    }

    for (const char* animLVar : kPaxDoorAnimLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, animLVar));
    }

    return status;
}

bool IFly737Max::IsPowered() const
{
    return variableGateway_->GetAVar(kSimAvionicsBusVoltage, kVoltsUnit, 0.0) > 0.0;
}

bool IFly737Max::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool IFly737Max::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool IFly737Max::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

bool IFly737Max::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool IFly737Max::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
}

bool IFly737Max::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateIFly737Max(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<IFly737Max>(context.variableGateway, context.status);
    }

    const AircraftDescriptor kIFly737MaxDescriptor{
        "iFly 737 MAX 8",
        {
            {MatchField::Title, MatchOp::Contains, "iFly 737-MAX"}
        },
        &CreateIFly737Max, "ifly-737max8", "B38M", RefuelBy::Gsx
    };

    [[maybe_unused]] const AircraftRegistration kIFly737MaxRegistration{kIFly737MaxDescriptor};
}
