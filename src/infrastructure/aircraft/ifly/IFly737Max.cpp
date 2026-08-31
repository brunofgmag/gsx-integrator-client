#include "IFly737Max.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <utility>
#include <memory>
#include <string>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../../domain/model/FlightPlan.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/ports/GsxGateway.h"
#include "../../../infrastructure/simvars/VariableGateway.h"

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

    constexpr std::array kCargoDoorAnimLVars =
        {"Animation_FWD_Cargo_VAL", "Animation_AFT_Cargo_VAL"};

    constexpr std::array kPaxDoorAnimLVars = {
        "ANIMATION_FWD_ENTRY_VAL", "ANIMATION_FWD_SERVICE_VAL",
        "ANIMATION_AFT_ENTRY_VAL", "ANIMATION_AFT_SERVICE_VAL",
        "ANIMATION_L_FWD_OVERWING_VAL", "ANIMATION_R_FWD_OVERWING_VAL",
        "ANIMATION_L_AFT_OVERWING_VAL", "ANIMATION_R_AFT_OVERWING_VAL"
    };
}

IFly737Max::IFly737Max(VariableGateway* variableGateway, const AutomationStatus* status,
                       std::optional<std::filesystem::path> planAppDataRoot)
    : variableGateway_(variableGateway), status_(status),
      smartSwitch_(*variableGateway, {kSmartSwitch},
                   [](const double min, const double max)
                   {
                       return min < kSmartSwitchNeutral || max > kSmartSwitchNeutral;
                   }),
      doorRule_(*variableGateway, *this),
      planImportRule_(planImport_, *status, std::move(planAppDataRoot)),
      rules_{&doorRule_, &planImportRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: iFly 737 MAX 8");
}

const std::vector<AircraftRule*>& IFly737Max::Rules() const
{
    return rules_;
}

void IFly737Max::CloseAllDoors()
{
    closeRequested_ = true;
}

void IFly737Max::HoldDoorsClosed(const bool hold)
{
    heldForDeparture_ = hold;
}

bool IFly737Max::IsHeldForDeparture() const
{
    return heldForDeparture_;
}

bool IFly737Max::WasCloseRequested() const
{
    return closeRequested_;
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
