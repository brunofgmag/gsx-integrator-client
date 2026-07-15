#include "TolissA340.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include "AircraftRegistry.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../domain/model/FlightPlan.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kKgUnit = "kg";

    constexpr std::array kMcduUplinkKeys = {"AB_MCDU3_MENU", "AB_MCDU3_LSK6L", "AB_MCDU3_LSK1R", "AB_MCDU3_LSK1L"};

    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr std::array kEngineFuelFlowLvars = {
        "TLS_ENG1_FUEL_FLOW", "TLS_ENG2_FUEL_FLOW", "TLS_ENG3_FUEL_FLOW", "TLS_ENG4_FUEL_FLOW"
    };
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kSmartSwitchLVar = "AB_ACP_CPT_RTU_Switch";
    constexpr double kSmartSwitchNeutral = 1.0;

    constexpr auto kParkingBrakeLVar = "PARKINGBRAKE_POSITION";
    constexpr double kParkingBrakeSetLVarValue = 100.0;

    constexpr auto kExtPowerAPbLVar = "AB_VC_OVH_ELEC_EXTA_PB";
    constexpr auto kExtPowerAAutoLVar = "AB_VC_OVH_ELEC_EXTA_AUTO";
    constexpr auto kExtPowerAOnLVar = "AB_VC_OVH_ELEC_EXTA_ON";
    constexpr auto kExtPowerBPbLVar = "AB_VC_OVH_ELEC_EXTB_PB";
    constexpr auto kExtPowerBAutoLVar = "AB_VC_OVH_ELEC_EXTB_AUTO";
    constexpr auto kExtPowerBOnLVar = "AB_VC_OVH_ELEC_EXTB_ON";
    constexpr double kExtPowerEnergizedValue = 10.0;

    constexpr auto kApuAvailLVar = "AB_VC_OVH_APU_START_AVAIL";
    constexpr double kApuAvailableValue = 10.0;

    constexpr auto kCargoDoorModeFwdLVar = "TLS_CARGO_DOOR_MODE_FWD";
    constexpr auto kCargoDoorModeAftLVar = "TLS_CARGO_DOOR_MODE_AFT";
    constexpr double kCargoDoorOpen = 2.0;
    constexpr double kCargoDoorClosed = 0.0;

    constexpr auto kPaxDoorMode1LLVar = "TLS_PAX_DOOR_MODE_1L";
    constexpr auto kPaxDoorMode2LLVar = "TLS_PAX_DOOR_MODE_2L";
    constexpr auto kPaxDoorMode4LLVar = "TLS_PAX_DOOR_MODE_4L";
    constexpr double kPaxDoorOpen = 2.0;
    constexpr double kPaxDoorClosed = 0.0;

    bool IsExternalPowerFeeding(VariableGateway* gateway,
                                const char* pbLVar,
                                const char* autoLVar,
                                const char* onLVar)
    {
        return gateway->GetLVar(onLVar, 0.0) == kExtPowerEnergizedValue
            || (gateway->GetLVar(pbLVar, 0.0) > 0.0
                && gateway->GetLVar(autoLVar, 0.0) == kExtPowerEnergizedValue);
    }
}

TolissA340::TolissA340(VariableGateway* variableGateway, AutomationStatus* status, const bool cargoVariant)
    : variableGateway_(variableGateway),
      status_(status),
      cargoVariant_(cargoVariant)
{
    variableGateway_->SetFastRefresh(std::string("L:") + kSmartSwitchLVar);

    LOG_INFO("Profile loaded: Toliss A340-600");
}

const char* TolissA340::GetName() const
{
    return kName;
}

bool TolissA340::IsCargoVariant() const
{
    return cargoVariant_;
}

void TolissA340::OnTick()
{
    UpdateCargoDoors();
    UpdatePaxDoors();

    if (!IsPowered())
    {
        return;
    }

    if (uplinkArmed_)
    {
        uplinkArmed_ = false;
        uplinkStep_ = 0;

        LOG_INFO("Starting SimBrief uplink through the center MCDU");
    }

    if (uplinkStep_ < 0 || uplinkStep_ >= static_cast<int>(kMcduUplinkKeys.size()))
    {
        uplinkStep_ = -1;
        return;
    }

    variableGateway_->SetLVar(kMcduUplinkKeys[uplinkStep_], 1.0);
    ++uplinkStep_;
}

void TolissA340::OnLoadingStarted()
{
    uplinkArmed_ = true;
    uplinkStep_ = -1;

    LOG_INFO("SimBrief uplink armed: waiting for the MCDU to be available");
}

void TolissA340::UpdateCargoDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    DriveCargoDoor(gsx::lvars::kBaggageLoaderFrontState, kCargoDoorModeFwdLVar, fwdCargoDoorTarget_);
    DriveCargoDoor(gsx::lvars::kBaggageLoaderRearState, kCargoDoorModeAftLVar, aftCargoDoorTarget_);
}

void TolissA340::DriveCargoDoor(const char* loaderStateLVar, const char* doorModeLVar, double& lastDoorTarget) const
{
    const double loaderState = variableGateway_->GetLVar(loaderStateLVar, 0.0);

    const double doorTarget =
        gsx::states::IsLoaderPresent(loaderState) ? kCargoDoorOpen : kCargoDoorClosed;

    if (doorTarget != lastDoorTarget)
    {
        variableGateway_->SetLVar(doorModeLVar, doorTarget);
        lastDoorTarget = doorTarget;
    }
}

void TolissA340::UpdatePaxDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    DrivePaxDoor(gsx::lvars::kPassengerStairsFrontState, kPaxDoorMode1LLVar, fwdPaxDoorTarget_);
    DrivePaxDoor(gsx::lvars::kPassengerStairsMiddleState, kPaxDoorMode2LLVar, midPaxDoorTarget_);
    DrivePaxDoor(gsx::lvars::kPassengerStairsRearState, kPaxDoorMode4LLVar, aftPaxDoorTarget_);
}

void TolissA340::DrivePaxDoor(const char* stairsStateLVar, const char* doorModeLVar, double& lastDoorTarget) const
{
    if (variableGateway_->GetLVar(stairsStateLVar, 0.0) == gsx::states::kStairsFinalPosition)
    {
        if (lastDoorTarget != kPaxDoorOpen)
        {
            variableGateway_->SetLVar(doorModeLVar, kPaxDoorOpen);
            lastDoorTarget = kPaxDoorOpen;
        }
    }
    else if (lastDoorTarget == kPaxDoorOpen)
    {
        variableGateway_->SetLVar(doorModeLVar, kPaxDoorClosed);
        lastDoorTarget = kPaxDoorClosed;
    }
}

bool TolissA340::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
}

double TolissA340::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double TolissA340::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int TolissA340::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double TolissA340::GetEmptyZfwKg() const
{
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

double TolissA340::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void TolissA340::SetCurrentFuelKg(double)
{
}

double TolissA340::GetCurrentZfwKg() const
{
    const double totalWeightKg =
        variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, GetEmptyZfwKg());

    return std::max(totalWeightKg - GetCurrentFuelKg(), GetEmptyZfwKg());
}

void TolissA340::SetCurrentZfwKg(double)
{
}

bool TolissA340::ConsumeSmartSwitch()
{
    const bool isActive =
        variableGateway_->GetLVar(kSmartSwitchLVar, kSmartSwitchNeutral) != kSmartSwitchNeutral;

    if (!isActive)
    {
        smartSwitchResetPending_ = false;
        return false;
    }

    variableGateway_->SetLVar(kSmartSwitchLVar, kSmartSwitchNeutral);

    if (smartSwitchResetPending_)
    {
        return false;
    }

    smartSwitchResetPending_ = true;

    return true;
}

bool TolissA340::IsPowered() const
{
    return IsExternalPowerOn() || variableGateway_->GetLVar(kApuAvailLVar, 0.0) == kApuAvailableValue;
}

bool TolissA340::IsExternalPowerOn() const
{
    return IsExternalPowerFeeding(variableGateway_, kExtPowerAPbLVar, kExtPowerAAutoLVar, kExtPowerAOnLVar)
        || IsExternalPowerFeeding(variableGateway_, kExtPowerBPbLVar, kExtPowerBAutoLVar, kExtPowerBOnLVar);
}

bool TolissA340::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool TolissA340::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsParkingBrakeSet() && !IsBeaconOn();
}

bool TolissA340::IsEngineRunning() const
{
    return std::ranges::any_of(kEngineFuelFlowLvars,
                               [this](const char* fuelFlowLVar)
                               {
                                   return variableGateway_->GetLVar(fuelFlowLVar, 1.0) > 0.0;
                               });
}

bool TolissA340::IsParkingBrakeSet() const
{
    return variableGateway_->GetAVar(kSimParkingBrake, kBoolUnit, 0.0) > 0.0
        || variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) >= kParkingBrakeSetLVarValue;
}

bool TolissA340::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateTolissA340(VariableGateway* variableGateway,
                                               AutomationStatus* status,
                                               const AircraftIdentity& identity)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "cargo");
        return std::make_unique<TolissA340>(variableGateway, status, cargo);
    }

    const AircraftDescriptor kTolissA340Descriptor{
        TolissA340::kName,
        {
            {MatchField::Title, MatchOp::Contains, "ToLiss A346"},
            {MatchField::Title, MatchOp::Contains, "Aerosoft A346"},
            {MatchField::AtcModel, MatchOp::Equals, "A346"}
        },
        &CreateTolissA340, "toliss-a340", "A346", RefuelBy::Self
    };

    [[maybe_unused]] const AircraftRegistration kTolissA340Registration{kTolissA340Descriptor};
}
