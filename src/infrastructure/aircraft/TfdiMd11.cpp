#include "TfdiMd11.h"

#include <algorithm>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "../../domain/support/Weight.h"
#include "../logging/LogMacros.h"
#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr auto kEfbGw = "MD11_EFB_PAYLOAD_GW";
    constexpr auto kEfbZfw = "MD11_EFB_PAYLOAD_ZFW";
    constexpr auto kEfbPayload = "MD11_EFB_PAYLOAD_PAYLOAD";
    constexpr auto kEfbFuel = "MD11_EFB_PAYLOAD_FUEL";
    constexpr auto kEfbLoad = "MD11_EFB_PAYLOAD_LOAD";
    constexpr auto kEfbReadReady = "MD11_EFB_READ_READY";

    constexpr int kEfbReadyMask = 0x1;
    constexpr double kMtowKg = 283730.0;

    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kKgUnit = "kg";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kSmartSwitch = "MD11_PED_CPT_AUDIO_PNL_INT_RADIO_SW";

    constexpr auto kPowerOnLVar = "MD11_CABIN_POWER";
    constexpr auto kBatteryOnLVar = "MD11_OVHD_ELEC_BATT_BT";
    constexpr auto kApuOnLVar = "MD11_OVHD_ELEC_APU_PWR_ON_LT";
    constexpr auto kExtPowerOnLightLVar = "MD11_OVHD_ELEC_EXT_PWR_ON_LT";
    constexpr auto kEng1N1LVar = "md11_eng1_n1";
    constexpr auto kEng2N1LVar = "md11_eng2_n1";
    constexpr auto kEng3N1LVar = "md11_eng3_n1";

    constexpr auto kBeaconBtnLVar = "MD11_OVHD_LTS_BCN_BT";
    constexpr auto kParkingBrakeLVar = "MD11_THR_PARK_LVR";
    constexpr auto kChocksLVar = "MD11_EXT_CHOCKS";
}

TfdiMd11::TfdiMd11(VariableGateway* variableGateway, AutomationStatus* status, const bool cargo)
    : variableGateway_(variableGateway), status_(status), cargo_(cargo)
{
    variableGateway_->SetFastRefresh(std::string("L:") + kSmartSwitch);

    LOG_INFO("Profile loaded: TFDi MD-11%s", cargo_ ? "F" : "");
}

const char* TfdiMd11::GetName() const
{
    return kName;
}

bool TfdiMd11::IsCargoVariant() const
{
    return cargo_;
}

void TfdiMd11::OnSlowTick()
{
    if (!pendingEfbCommit_)
    {
        return;
    }

    SeedTargetsIfNeeded();
    CommitEfbTargets();
    pendingEfbCommit_ = false;
}

void TfdiMd11::SeedTargetsIfNeeded()
{
    if (!fuelTargetSeeded_ && variableGateway_->HasReceivedAVar(kSimFuelTotalKg, kKgUnit))
    {
        targetFuelKg_ = GetCurrentFuelKg();
        fuelTargetSeeded_ = true;
    }

    if (!zfwTargetSeeded_ && variableGateway_->HasReceivedAVar(kSimTotalWeight, kKgUnit))
    {
        targetZfwKg_ = std::max(GetCurrentZfwKg(), GetEmptyZfwKg());
        zfwTargetSeeded_ = true;
    }
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
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

double TfdiMd11::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void TfdiMd11::SetCurrentFuelKg(const double fuelKg)
{
    if (lastFuelKg_ == fuelKg)
    {
        return;
    }

    lastFuelKg_ = fuelKg;
    targetFuelKg_ = fuelKg;
    fuelTargetSeeded_ = true;
    pendingEfbCommit_ = true;
}

double TfdiMd11::GetCurrentZfwKg() const
{
    const double totalWeightKg =
        variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, GetEmptyZfwKg());

    return std::max(totalWeightKg - GetCurrentFuelKg(), GetEmptyZfwKg());
}

void TfdiMd11::SetCurrentZfwKg(const double zfwKg)
{
    if (lastZfwKg_ == zfwKg)
    {
        return;
    }

    lastZfwKg_ = zfwKg;
    targetZfwKg_ = zfwKg;
    zfwTargetSeeded_ = true;
    pendingEfbCommit_ = true;
}

bool TfdiMd11::ConsumeSmartSwitch()
{
    const bool isActive = variableGateway_->GetLVar(kSmartSwitch, 1.0) > 1.0;

    if (!isActive)
    {
        smartSwitchResetPending_ = false;
        return false;
    }

    variableGateway_->SetLVar(kSmartSwitch, 1.0);

    if (smartSwitchResetPending_)
    {
        return false;
    }

    smartSwitchResetPending_ = true;
    return true;
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

bool TfdiMd11::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool TfdiMd11::IsReadyToDeboard() const
{
    const bool isChocksOn = variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;

    return !IsEngineRunning() && (IsParkingBrakeSet() || isChocksOn) && !IsBeaconOn();
}

bool TfdiMd11::IsEngineRunning() const
{
    const bool isEng1Running = variableGateway_->GetLVar(kEng1N1LVar, 100.0) > 3.0;
    const bool isEng2Running = variableGateway_->GetLVar(kEng2N1LVar, 100.0) > 3.0;
    const bool isEng3Running = variableGateway_->GetLVar(kEng3N1LVar, 100.0) > 3.0;

    return isEng1Running || isEng2Running || isEng3Running;
}

bool TfdiMd11::IsParkingBrakeSet() const
{
    const bool isParkingBrakeOn = variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
    const bool isSimParkingBrakeOn = variableGateway_->GetAVar(kSimParkingBrake, kBoolUnit, 0.0) > 0.0;

    return isParkingBrakeOn && isSimParkingBrakeOn;
}

bool TfdiMd11::IsBeaconOn() const
{
    return variableGateway_->GetLVar(kBeaconBtnLVar, 0.0) > 0.0;
}

void TfdiMd11::CommitEfbTargets() const
{
    const double zfw = std::max(targetZfwKg_, GetEmptyZfwKg());
    const double fuel = std::max(targetFuelKg_, 0.0);
    const double payload = std::max(zfw - GetEmptyZfwKg(), 0.0);
    const double grossWeight = zfw + fuel;
    const double payloadCapacityKg = kMtowKg - GetEmptyZfwKg();
    const double loadPercentage = std::clamp(
        payload / payloadCapacityKg * 100.0,
        0.0,
        100.0);

    variableGateway_->SetLVar(kEfbGw, weight::KgToLb(grossWeight));
    variableGateway_->SetLVar(kEfbZfw, weight::KgToLb(zfw));
    variableGateway_->SetLVar(kEfbPayload, weight::KgToLb(payload));
    variableGateway_->SetLVar(kEfbFuel, weight::KgToLb(fuel));
    variableGateway_->SetLVar(kEfbLoad, loadPercentage);

    const int flags =
        static_cast<int>(variableGateway_->GetLVar(kEfbReadReady, 0.0))
        | kEfbReadyMask;
    variableGateway_->SetLVar(kEfbReadReady, flags);

    LOG_INFO(
        "Committed EFB targets: fuel=%.0fkg zfw=%.0fkg payload=%.0fkg",
        fuel,
        zfw,
        payload);
}

namespace
{
    std::unique_ptr<Aircraft> CreateTfdiMd11(VariableGateway* variableGateway,
                                             AutomationStatus* status,
                                             const AircraftIdentity& identity)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "MD-11F")
            || MatchText(identity.atcModel, MatchOp::Equals, "MD11F");
        return std::make_unique<TfdiMd11>(variableGateway, status, cargo);
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
