#include "IFly737Max.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "../logging/LogMacros.h"
#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/AutomationStatus.h"
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
}

IFly737Max::IFly737Max(VariableGateway* variableGateway, AutomationStatus* status)
    : variableGateway_(variableGateway), status_(status)
{
    variableGateway_->SetFastRefresh(std::string("L:") + kSmartSwitch);

    LOG_INFO("Profile loaded: iFly 737 MAX 8");
}

const char* IFly737Max::GetName() const
{
    return "iFly 737 MAX 8";
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
    const bool isActive =
        variableGateway_->GetLVar(kSmartSwitch, kSmartSwitchNeutral) != kSmartSwitchNeutral;

    if (!isActive)
    {
        smartSwitchPressPending_ = false;
        return false;
    }

    if (smartSwitchPressPending_)
    {
        return false;
    }

    smartSwitchPressPending_ = true;

    return true;
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
    std::unique_ptr<Aircraft> CreateIFly737Max(VariableGateway* variableGateway,
                                               AutomationStatus* status,
                                               const AircraftIdentity&)
    {
        return std::make_unique<IFly737Max>(variableGateway, status);
    }

    const AircraftDescriptor kIFly737MaxDescriptor{
        "iFly 737 MAX 8",
        {
            {MatchField::Title, MatchOp::Contains, "iFly 737-MAX"}
        },
        &CreateIFly737Max
    };

    [[maybe_unused]] const AircraftRegistration kIFly737MaxRegistration{kIFly737MaxDescriptor};
}
