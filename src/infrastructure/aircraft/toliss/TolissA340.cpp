#include "TolissA340.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <optional>
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

    constexpr std::array kMcduUplinkKeys = {"AB_MCDU3_MENU", "AB_MCDU3_LSK6L", "AB_MCDU3_LSK1R", "AB_MCDU3_LSK1L"};

    constexpr std::array kEngineFuelFlowLvars = {
        "TLS_ENG1_FUEL_FLOW", "TLS_ENG2_FUEL_FLOW", "TLS_ENG3_FUEL_FLOW", "TLS_ENG4_FUEL_FLOW"
    };

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

    constexpr auto kPaxDoorMode1LLVar = "TLS_PAX_DOOR_MODE_1L";
    constexpr auto kPaxDoorMode2LLVar = "TLS_PAX_DOOR_MODE_2L";
    constexpr auto kPaxDoorMode3LLVar = "TLS_PAX_DOOR_MODE_3L";
    constexpr auto kPaxDoorMode4LLVar = "TLS_PAX_DOOR_MODE_4L";
    constexpr auto kPaxDoorMode1RLVar = "TLS_PAX_DOOR_MODE_1R";
    constexpr auto kPaxDoorMode2RLVar = "TLS_PAX_DOOR_MODE_2R";
    constexpr auto kPaxDoorMode3RLVar = "TLS_PAX_DOOR_MODE_3R";
    constexpr auto kPaxDoorMode4RLVar = "TLS_PAX_DOOR_MODE_4R";
    constexpr double kDoorOpen = 2.0;
    constexpr double kDoorClosed = 0.0;

    constexpr std::array kDoorModeLVars =
        {kCargoDoorModeFwdLVar, kCargoDoorModeAftLVar,
         kPaxDoorMode1LLVar, kPaxDoorMode2LLVar, kPaxDoorMode3LLVar, kPaxDoorMode4LLVar,
         kPaxDoorMode1RLVar, kPaxDoorMode2RLVar, kPaxDoorMode3RLVar, kPaxDoorMode4RLVar};

    constexpr std::array kDoorOpenRatioLVars =
        {"TLS_CARGO_DOOR_OPEN_RATIO_FWD", "TLS_CARGO_DOOR_OPEN_RATIO_AFT",
         "TLS_PAX_DOOR_OPEN_RATIO_1L", "TLS_PAX_DOOR_OPEN_RATIO_2L",
         "TLS_PAX_DOOR_OPEN_RATIO_3L", "TLS_PAX_DOOR_OPEN_RATIO_4L",
         "TLS_PAX_DOOR_OPEN_RATIO_1R", "TLS_PAX_DOOR_OPEN_RATIO_2R",
         "TLS_PAX_DOOR_OPEN_RATIO_3R", "TLS_PAX_DOOR_OPEN_RATIO_4R"};

    constexpr double kDoorSeatedRatio = 0.0;

    std::optional<bool> DoorOpenByMode(VariableGateway& variables, const char* modeLVar)
    {
        if (!variables.HasReceivedLVar(modeLVar))
        {
            return std::nullopt;
        }

        const double mode = variables.GetLVar(modeLVar, kDoorOpen);
        if (mode == kDoorClosed)
        {
            return false;
        }

        return mode == kDoorOpen ? std::optional{true} : std::nullopt;
    }

    std::optional<bool> DoorOpen(VariableGateway& variables, const char* modeLVar, const char* ratioLVar)
    {
        if (variables.HasReceivedLVar(ratioLVar))
        {
            return variables.GetLVar(ratioLVar, kDoorSeatedRatio) > kDoorSeatedRatio;
        }

        return DoorOpenByMode(variables, modeLVar);
    }

    const char* DoorModeLVar(const GsxDoor door)
    {
        switch (door)
        {
        case GsxDoor::FwdPax:
            return kPaxDoorMode1LLVar;
        case GsxDoor::MidPax:
            return kPaxDoorMode2LLVar;
        case GsxDoor::AftPax:
            return kPaxDoorMode4LLVar;
        case GsxDoor::FwdCatering:
            return kPaxDoorMode1RLVar;
        case GsxDoor::AftCatering:
            return kPaxDoorMode4RLVar;
        case GsxDoor::FwdCargo:
            return kCargoDoorModeFwdLVar;
        default:
            return kCargoDoorModeAftLVar;
        }
    }

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

TolissA340::TolissA340(VariableGateway* variableGateway, const AutomationStatus* status, const bool cargoVariant)
    : variableGateway_(variableGateway),
      status_(status),
      cargoVariant_(cargoVariant),
      doors_(variableGateway),
      smartSwitch_(*variableGateway, {kSmartSwitchLVar},
                   [](const double min, const double max)
                   {
                       return min < kSmartSwitchNeutral || max > kSmartSwitchNeutral;
                   },
                   kSmartSwitchNeutral)
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: Toliss A340-600");
}

bool TolissA340::IsCargoVariant() const
{
    return cargoVariant_;
}

void TolissA340::Observe()
{
    doors_.Observe();
}

void TolissA340::OnTick()
{
    Observe();
    UpdateDoors();

    if (!IsPowered())
    {
        return;
    }

    AdvanceUplink();
}

void TolissA340::AdvanceUplink()
{
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

DoorStatus TolissA340::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (std::size_t door = 0; door < kDoorModeLVars.size(); ++door)
    {
        status = doors::Combine(status, DoorOpen(*variableGateway_, kDoorModeLVars[door],
                                                 kDoorOpenRatioLVars[door]));
    }

    return status;
}

void TolissA340::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, bool)
    {
        variableGateway_->SetLVar(DoorModeLVar(door), kDoorClosed);
    });

    variableGateway_->SetLVar(kPaxDoorMode3LLVar, kDoorClosed);
    variableGateway_->SetLVar(kPaxDoorMode2RLVar, kDoorClosed);
    variableGateway_->SetLVar(kPaxDoorMode3RLVar, kDoorClosed);

    LOG_INFO("All doors commanded closed: door control is now manual");
}

void TolissA340::UpdateDoors()
{
    doors_.Sync([this](const GsxDoor door, const bool open)
    {
        variableGateway_->SetLVar(DoorModeLVar(door), open ? kDoorOpen : kDoorClosed);
    });
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
    return EmptyZfwKg(*variableGateway_);
}

double TolissA340::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double TolissA340::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

bool TolissA340::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
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
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool TolissA340::IsHeldInPlace() const
{
    return IsParkingBrakeSet();
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
    return variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) >= kParkingBrakeSetLVarValue;
}

bool TolissA340::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateTolissA340(const AircraftContext& context, const AircraftIdentity& identity)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "cargo");
        return std::make_unique<TolissA340>(context.variableGateway, context.status, cargo);
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

void TolissA340::HoldDoorsClosed(const bool hold)
{
    doors_.HoldClosedForDeparture(hold);
}
