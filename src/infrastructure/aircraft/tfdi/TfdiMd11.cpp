#include "TfdiMd11.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <memory>
#include <string>
#include <array>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../../domain/model/FlightPlan.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kSmartSwitch = "MD11_PED_CPT_AUDIO_PNL_INT_RADIO_SW";
    constexpr double kSmartSwitchNeutral = 1.0;

    constexpr auto kPowerOnLVar = "MD11_CABIN_POWER";
    constexpr auto kBatteryOnLVar = "MD11_OVHD_ELEC_BATT_BT";
    constexpr auto kApuOnLVar = "MD11_OVHD_ELEC_APU_PWR_ON_LT";
    constexpr auto kExtPowerOnLightLVar = "MD11_OVHD_ELEC_EXT_PWR_ON_LT";

    constexpr int kEngineCount = 3;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kParkingBrakeLVar = "MD11_THR_PARK_LVR";
    constexpr auto kChocksLVar = "MD11_EXT_CHOCKS";
    constexpr auto kExtGpuLVar = "MD11_EXT_GPU";

    constexpr std::array kDoorStateLVars =
        {"MD11_EXT_DOOR_PAX_1L", "MD11_EXT_DOOR_PAX_2L", "MD11_EXT_DOOR_PAX_4L",
         "MD11_EXT_DOOR_CARGO_1R", "MD11_EXT_DOOR_CARGO_2R", "MD11_EXT_DOOR_CARGO_MAIN"};
}

TfdiMd11::TfdiMd11(VariableGateway* variableGateway, const AutomationStatus* status, const bool cargo)
    : variableGateway_(variableGateway), status_(status), cargo_(cargo),
      smartSwitch_(*variableGateway, {kSmartSwitch},
                   [](double, const double max) { return max > kSmartSwitchNeutral; },
                   kSmartSwitchNeutral),
      cargoDoorRule_(*variableGateway, cargo),
      paxDoorRule_(*variableGateway),
      efbTargetRule_(*variableGateway, *this),
      rules_{&cargoDoorRule_, &paxDoorRule_, &efbTargetRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: TFDi MD-11%s", cargo_ ? "F" : "");
}

bool TfdiMd11::IsCargoVariant() const
{
    return cargo_;
}

const std::vector<AircraftRule*>& TfdiMd11::Rules() const
{
    return rules_;
}

bool TfdiMd11::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
}

double TfdiMd11::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double TfdiMd11::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int TfdiMd11::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double TfdiMd11::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double TfdiMd11::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

void TfdiMd11::SetCurrentFuelKg(const double fuelKg)
{
    stagedFuelKg_ = fuelKg;
}

std::optional<double> TfdiMd11::StagedFuelKg() const
{
    return stagedFuelKg_;
}

double TfdiMd11::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

void TfdiMd11::SetCurrentZfwKg(const double zfwKg)
{
    stagedZfwKg_ = zfwKg;
}

std::optional<double> TfdiMd11::StagedZfwKg() const
{
    return stagedZfwKg_;
}

DoorStatus TfdiMd11::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* stateLVar : kDoorStateLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, stateLVar));
    }

    return status;
}

bool TfdiMd11::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool TfdiMd11::IsPowered() const
{
    const bool isPowered = variableGateway_->GetLVar(kPowerOnLVar, 0.0) > 0.0;
    const bool isExtPowered = variableGateway_->GetLVar(kExtPowerOnLightLVar, 0.0) > 0.0;
    const bool isApuPowered = variableGateway_->GetLVar(kApuOnLVar, 0.0) > 0.0;
    const bool isBatteryPowered = variableGateway_->GetLVar(kBatteryOnLVar, 0.0) > 0.0;

    if (!isPowered && !isBatteryPowered)
    {
        return false;
    }

    if (isBatteryPowered && !isExtPowered && !isApuPowered)
    {
        return false;
    }

    return true;
}

std::optional<GroundPowerStatus> TfdiMd11::GetGroundPowerStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kExtGpuLVar))
    {
        return GroundPowerStatus::Unknown;
    }

    return variableGateway_->GetLVar(kExtGpuLVar, 0.0) > 0.0
               ? GroundPowerStatus::Connected
               : GroundPowerStatus::Disconnected;
}

bool TfdiMd11::SetChocks(const bool placed)
{
    variableGateway_->SetLVar(kChocksLVar, placed ? 1.0 : 0.0);

    return true;
}

bool TfdiMd11::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool TfdiMd11::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool TfdiMd11::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

bool TfdiMd11::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool TfdiMd11::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
}

bool TfdiMd11::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateTfdiMd11(const AircraftContext& context, const AircraftIdentity& identity)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "MD-11F")
            || MatchText(identity.atcModel, MatchOp::Equals, "MD11F");
        return std::make_unique<TfdiMd11>(context.variableGateway, context.status, cargo);
    }

    const AircraftDescriptor kTfdiMd11Descriptor{
        TfdiMd11::kName,
        {
            {MatchField::Title, MatchOp::Contains, "TFDi Design MD-11"},
            {MatchField::AtcModel, MatchOp::Equals, "MD11"},
            {MatchField::AtcModel, MatchOp::Equals, "MD11F"}
        },
        &CreateTfdiMd11, "tfdi-md11", "MD11", RefuelBy::Self
    };

    [[maybe_unused]] const AircraftRegistration kTfdiMd11Registration{kTfdiMd11Descriptor};
}
