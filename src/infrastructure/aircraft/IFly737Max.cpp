#include "IFly737Max.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/ports/GsxGateway.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimAvionicsBusVoltage = "ELECTRICAL AVIONICS BUS VOLTAGE";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kKgUnit = "kg";
    constexpr auto kBoolUnit = "Bool";
    constexpr auto kVoltsUnit = "Volts";

    constexpr auto kSmartSwitch = "VC_ACP_1_Push_to_Talk_SW_VAL";
    constexpr double kSmartSwitchNeutral = 10.0;

    constexpr auto kParkingBrakeLVar = "VC_Parking_Brake_SW_VAL";
    constexpr auto kChocksLVar = "iFly_NLG_Chock_Display_VAL";

    constexpr auto kSimPayloadStationPrefix = "PAYLOAD STATION WEIGHT:";
    constexpr std::array kStationDefaultLoadsLbs =
        {2100.0, 5250.0, 1050.0, 2975.0, 3150.0, 3500.0, 2275.0, 5752.0, 8018.0};

    constexpr auto kFwdCargoAnimLVar = "Animation_FWD_Cargo_VAL";
    constexpr auto kAftCargoAnimLVar = "Animation_AFT_Cargo_VAL";
    constexpr double kCargoDoorOpenThreshold = 90.0;
    constexpr int kPulseSettleTicks = 5;
    constexpr int kMaxDoorPulseAttempts = 3;

    bool IsState(const double lvarValue, const GsxStateStatus state)
    {
        return lvarValue == static_cast<double>(state);
    }
}

IFly737Max::IFly737Max(VariableGateway* variableGateway, AutomationStatus* status)
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
      }
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: iFly 737 MAX 8");
}

const char* IFly737Max::GetName() const
{
    return "iFly 737 MAX 8";
}

void IFly737Max::OnTick()
{
    CloseCargoDoorsAfterUnloading();
}

void IFly737Max::CloseCargoDoorsAfterUnloading()
{
    const double deboarding = variableGateway_->GetLVar(gsx::lvars::kDeboardingState, 0.0);
    const bool deboardActive = IsState(deboarding, GsxStateStatus::Active);

    if (deboardActive && !cargoDoorCloseArmed_)
    {
        ArmCargoDoorCloser();
    }

    if (!cargoDoorCloseArmed_)
    {
        return;
    }

    if (IsBoardingUnderway())
    {
        DisarmCargoDoorCloser();

        return;
    }

    TrackBaggageLoader(fwdCargoDoor_);
    TrackBaggageLoader(aftCargoDoor_);

    if (AdvanceDoorPulse())
    {
        return;
    }

    if (!deboardActive && !HasPendingCargoDoorWork())
    {
        cargoDoorCloseArmed_ = false;
    }
}

void IFly737Max::ArmCargoDoorCloser()
{
    cargoDoorCloseArmed_ = true;
    ResetDoorTracking(fwdCargoDoor_);
    ResetDoorTracking(aftCargoDoor_);
}

void IFly737Max::ResetDoorTracking(CargoDoorCloser& door)
{
    door.unloadingSeen = false;
    door.loaderDone = false;
    door.attempts = 0;
}

bool IFly737Max::AdvanceDoorPulse()
{
    if (pulseHighDoor_ != nullptr)
    {
        variableGateway_->SetLVar(pulseHighDoor_->toggleLVar, 0.0);
        pulseHighDoor_ = nullptr;
        pulseSettleTicks_ = kPulseSettleTicks;

        return true;
    }

    if (pulseSettleTicks_ > 0)
    {
        --pulseSettleTicks_;

        return true;
    }

    CargoDoorCloser* door = NextCloseableDoor();
    if (door == nullptr)
    {
        return false;
    }

    ++door->attempts;
    variableGateway_->SetLVar(door->toggleLVar, 1.0);
    pulseHighDoor_ = door;

    LOG_INFO("iFly: closing %s cargo door behind its baggage loader (attempt %d/%d)",
             door->doorName, door->attempts, kMaxDoorPulseAttempts);

    return true;
}

bool IFly737Max::IsBoardingUnderway() const
{
    const double boarding = variableGateway_->GetLVar(gsx::lvars::kBoardingState, 0.0);

    return IsState(boarding, GsxStateStatus::Requested) || IsState(boarding, GsxStateStatus::Active);
}

bool IFly737Max::HasPendingCargoDoorWork() const
{
    return IsBaggageLoaderPresent(fwdCargoDoor_.loaderLVar)
        || IsBaggageLoaderPresent(aftCargoDoor_.loaderLVar)
        || IsDoorClosePending(fwdCargoDoor_)
        || IsDoorClosePending(aftCargoDoor_);
}

void IFly737Max::TrackBaggageLoader(CargoDoorCloser& door) const
{
    const double loaderState = variableGateway_->GetLVar(door.loaderLVar, 0.0);

    if (loaderState == gsx::states::kLoaderUnloading)
    {
        door.unloadingSeen = true;
    }

    if (gsx::states::IsLoaderAtDoor(loaderState) || loaderState == gsx::states::kLoaderRetracting)
    {
        door.loaderDone = false;
        door.attempts = 0;

        return;
    }

    if (door.unloadingSeen)
    {
        door.loaderDone = true;
    }
}

bool IFly737Max::IsBaggageLoaderPresent(const char* loaderLVar) const
{
    return variableGateway_->HasReceivedLVar(loaderLVar)
        && gsx::states::IsLoaderPresent(variableGateway_->GetLVar(loaderLVar, 0.0));
}

bool IFly737Max::IsDoorCloseable(const CargoDoorCloser& door) const
{
    return door.loaderDone
        && door.attempts < kMaxDoorPulseAttempts
        && variableGateway_->GetLVar(door.animLVar, 0.0) > kCargoDoorOpenThreshold;
}

bool IFly737Max::IsDoorClosePending(const CargoDoorCloser& door) const
{
    return (door.unloadingSeen && !door.loaderDone) || IsDoorCloseable(door);
}

IFly737Max::CargoDoorCloser* IFly737Max::NextCloseableDoor()
{
    for (CargoDoorCloser* door : {&fwdCargoDoor_, &aftCargoDoor_})
    {
        if (IsDoorCloseable(*door))
        {
            return door;
        }
    }

    return nullptr;
}

void IFly737Max::DisarmCargoDoorCloser()
{
    if (pulseHighDoor_ != nullptr)
    {
        variableGateway_->SetLVar(pulseHighDoor_->toggleLVar, 0.0);
        pulseHighDoor_ = nullptr;
    }

    cargoDoorCloseArmed_ = false;
    pulseSettleTicks_ = 0;
}

bool IFly737Max::IsCargoVariant() const
{
    return false;
}

bool IFly737Max::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
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
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

double IFly737Max::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void IFly737Max::SetCurrentFuelKg(double)
{
}

double IFly737Max::GetCurrentZfwKg() const
{
    const double totalWeightKg =
        variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, GetEmptyZfwKg());

    return std::max(totalWeightKg - GetCurrentFuelKg(), GetEmptyZfwKg());
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
    const bool isChocksOn = variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;

    return !IsEngineRunning() && (IsParkingBrakeSet() || isChocksOn) && !IsBeaconOn();
}

bool IFly737Max::IsEngineRunning() const
{
    const bool isEng1Running = variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng2Running = variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 1.0) > 0.0;

    return isEng1Running || isEng2Running;
}

bool IFly737Max::IsParkingBrakeSet() const
{
    const bool isParkingBrakeOn = variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
    const bool isSimParkingBrakeOn = variableGateway_->GetAVar(kSimParkingBrake, kBoolUnit, 0.0) > 0.0;

    return isParkingBrakeOn && isSimParkingBrakeOn;
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
