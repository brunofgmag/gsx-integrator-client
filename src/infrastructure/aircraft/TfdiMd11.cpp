#include "TfdiMd11.h"

#include <algorithm>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "../gsx/GsxLVars.h"
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
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kSimEng3Combustion = "ENG COMBUSTION:3";
    constexpr auto kKgUnit = "kg";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kSmartSwitch = "MD11_PED_CPT_AUDIO_PNL_INT_RADIO_SW";
    constexpr double kSmartSwitchNeutral = 1.0;

    constexpr auto kPowerOnLVar = "MD11_CABIN_POWER";
    constexpr auto kBatteryOnLVar = "MD11_OVHD_ELEC_BATT_BT";
    constexpr auto kApuOnLVar = "MD11_OVHD_ELEC_APU_PWR_ON_LT";
    constexpr auto kExtPowerOnLightLVar = "MD11_OVHD_ELEC_EXT_PWR_ON_LT";

    constexpr auto kParkingBrakeLVar = "MD11_THR_PARK_LVR";
    constexpr auto kChocksLVar = "MD11_EXT_CHOCKS";
    constexpr auto kExtGpuLVar = "MD11_EXT_GPU";

    constexpr auto kPaxDoor1LLVar = "MD11_EXT_DOOR_CMD_PAX_1L";
    constexpr auto kPaxDoor2LLVar = "MD11_EXT_DOOR_CMD_PAX_2L";
    constexpr auto kPaxDoor4LLVar = "MD11_EXT_DOOR_CMD_PAX_4L";
    constexpr auto kCargoDoor1RLVar = "MD11_EXT_DOOR_CMD_CARGO1R";
    constexpr auto kCargoDoor2RLVar = "MD11_EXT_DOOR_CMD_CARGO2R";
    constexpr auto kCargoDoorMainLVar = "MD11_EXT_DOOR_CMD_CARGO_MAIN";
    constexpr double kDoorOpen = 100.0;
    constexpr double kDoorClosed = 0.0;
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

void TfdiMd11::OnTick()
{
    UpdateCargoDoors();
    UpdatePaxDoors();
}

void TfdiMd11::UpdateCargoDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    DriveLoaderDoor(gsx::lvars::kBaggageLoaderFrontState, kCargoDoor1RLVar, fwdCargoDoorTarget_);
    DriveLoaderDoor(gsx::lvars::kBaggageLoaderRearState, kCargoDoor2RLVar, aftCargoDoorTarget_);

    if (cargo_)
    {
        DriveLoaderDoor(gsx::lvars::kBaggageLoaderMainState, kCargoDoorMainLVar, mainCargoDoorTarget_);
    }
}

void TfdiMd11::DriveLoaderDoor(const char* loaderStateLVar, const char* doorCmdLVar, double& lastDoorTarget) const
{
    const double loaderState = variableGateway_->GetLVar(loaderStateLVar, 0.0);
    const double doorTarget = gsx::states::IsLoaderPresent(loaderState) ? kDoorOpen : kDoorClosed;

    if (doorTarget != lastDoorTarget)
    {
        variableGateway_->SetLVar(doorCmdLVar, doorTarget);
        lastDoorTarget = doorTarget;
    }
}

void TfdiMd11::UpdatePaxDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    DriveStairsDoor(gsx::lvars::kPassengerStairsFrontState, kPaxDoor1LLVar, fwdPaxDoorTarget_);
    DriveStairsDoor(gsx::lvars::kPassengerStairsMiddleState, kPaxDoor2LLVar, midPaxDoorTarget_);
    DriveStairsDoor(gsx::lvars::kPassengerStairsRearState, kPaxDoor4LLVar, aftPaxDoorTarget_);
}

void TfdiMd11::DriveStairsDoor(const char* stairsStateLVar, const char* doorCmdLVar, double& lastDoorTarget) const
{
    if (variableGateway_->GetLVar(stairsStateLVar, 0.0) == gsx::states::kStairsFinalPosition)
    {
        if (lastDoorTarget != kDoorOpen)
        {
            variableGateway_->SetLVar(doorCmdLVar, kDoorOpen);
            lastDoorTarget = kDoorOpen;
        }
    }
    else if (lastDoorTarget == kDoorOpen)
    {
        variableGateway_->SetLVar(doorCmdLVar, kDoorClosed);
        lastDoorTarget = kDoorClosed;
    }
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
    if (!fuelTarget_.seeded && variableGateway_->HasReceivedAVar(kSimFuelTotalKg, kKgUnit))
    {
        fuelTarget_.target = GetCurrentFuelKg();
        fuelTarget_.seeded = true;
    }

    if (!zfwTarget_.seeded && variableGateway_->HasReceivedAVar(kSimTotalWeight, kKgUnit))
    {
        zfwTarget_.target = std::max(GetCurrentZfwKg(), GetEmptyZfwKg());
        zfwTarget_.seeded = true;
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
    UpdateTarget(fuelTarget_, fuelKg);
}

void TfdiMd11::UpdateTarget(EfbTarget& efbTarget, const double valueKg)
{
    if (efbTarget.last == valueKg)
    {
        return;
    }

    efbTarget.last = valueKg;
    efbTarget.target = valueKg;
    efbTarget.seeded = true;
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
    UpdateTarget(zfwTarget_, zfwKg);
}

bool TfdiMd11::ConsumeSmartSwitch()
{
    const bool isActive = variableGateway_->GetLVar(kSmartSwitch, kSmartSwitchNeutral) > kSmartSwitchNeutral;

    if (!isActive)
    {
        smartSwitchResetPending_ = false;
        return false;
    }

    variableGateway_->SetLVar(kSmartSwitch, kSmartSwitchNeutral);

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

void TfdiMd11::SetChocks(const bool placed)
{
    variableGateway_->SetLVar(kChocksLVar, placed ? 1.0 : 0.0);
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
    const bool isEng1Running = variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng2Running = variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng3Running = variableGateway_->GetAVar(kSimEng3Combustion, kBoolUnit, 1.0) > 0.0;

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
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

void TfdiMd11::CommitEfbTargets() const
{
    const double zfw = std::max(zfwTarget_.target, GetEmptyZfwKg());
    const double fuel = std::max(fuelTarget_.target, 0.0);
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
