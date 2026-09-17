#include "FssEJet.h"

#include "../../simvars/SimVars.h"

#include <array>
#include <memory>
#include <string>

#include "../AircraftRegistry.h"
#include "../../logging/LogMacros.h"
#include "../../simvars/VariableGateway.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/model/FlightPlan.h"

using namespace simvars;

namespace
{
    constexpr auto kElecPwrAcAvailLVar = "FSS_EXX_ELEC_PWR_AC_AVAIL";
    constexpr auto kBeaconSwitchLVar = "FSS_EXX_OVHD_EXLT_RED_BCN_SWITCH";
    constexpr auto kParkBrakeLeverLVar = "FSS_EXX_PARKBRAKE_BV_LEVER";

    constexpr auto kCallRampLeftLVar = "FSS_EXX_AUDIO_L_TEL_RAMP_BTN";
    constexpr auto kCallRampRightLVar = "FSS_EXX_AUDIO_R_TEL_RAMP_BTN";
    constexpr double kCallRampOff = 0.0;

    constexpr auto kGpuStateLVar = "FSS_EXX_EXT_GPU_STATE";
    constexpr double kGpuStateHidden = -1.0;
    constexpr double kGpuStateInactive = 0.0;
    constexpr double kGpuStateFeeding = 5.0;

    constexpr std::array kChocksLVars = {
        "FSS_EXX_GNDOBJ_WHEEL_CHOKE_F", "FSS_EXX_GNDOBJ_WHEEL_CHOKE_L", "FSS_EXX_GNDOBJ_WHEEL_CHOKE_R"
    };
    constexpr double kChocksPlaced = 1.0;
    constexpr double kChocksStowed = 0.0;

    constexpr std::array kOwnGroundEquipmentLVars = {
        "FSS_EXX_GNDOBJ_CONE_ENG_L", "FSS_EXX_GNDOBJ_CONE_ENG_R", "FSS_EXX_GNDOBJ_CONE_ENTRY",
        "FSS_EXX_GNDOBJ_CONE_TAIL", "FSS_EXX_GNDOBJ_CONE_WING_L", "FSS_EXX_GNDOBJ_CONE_WING_R",
        "FSS_EXX_GNDOBJ_ENG_COVER_L", "FSS_EXX_GNDOBJ_ENG_COVER_R",
        "FSS_EXX_WHEEL_COVER_L", "FSS_EXX_WHEEL_COVER_R",
        "FSS_EXX_SAFETY_PIN_LDGGEAR_L", "FSS_EXX_SAFETY_PIN_LDGGEAR_R", "FSS_EXX_SAFETY_PIN_PITOT_BOTTOM",
        "FSS_EXX_SAFETY_PIN_PITOT_F_L", "FSS_EXX_SAFETY_PIN_PITOT_F_R",
        "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_L", "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_R",
        "FSS_EXX_STAIR_FWD_L_ACTIVE", "FSS_EXX_STAIR_AFT_L_ACTIVE"
    };
    constexpr double kEquipmentStowed = 0.0;

    constexpr int kEngineCount = 2;
    constexpr double kEngineRunningDefault = 1.0;
}

FssEJet::FssEJet(VariableGateway* variableGateway, const AutomationStatus* status, const char* name,
                 const bool cargoVariant)
    : variableGateway_(variableGateway),
      status_(status),
      cargoVariant_(cargoVariant),
      smartSwitch_(*variableGateway, {kCallRampLeftLVar, kCallRampRightLVar},
                   [](double, const double max)
                   {
                       return max > kCallRampOff;
                   },
                   kCallRampOff),
      automationRule_(*variableGateway),
      gpuRule_(*this),
      rules_{&automationRule_, &gpuRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: %s%s", name, cargoVariant ? " Freighter" : "");
}

bool FssEJet::IsCargoVariant() const
{
    return cargoVariant_;
}

const std::vector<AircraftRule*>& FssEJet::Rules() const
{
    return rules_;
}

bool FssEJet::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
}

double FssEJet::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double FssEJet::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int FssEJet::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double FssEJet::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double FssEJet::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double FssEJet::GetCurrentZfwKg() const
{
    return CurrentZfwKg(*variableGateway_);
}

bool FssEJet::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

std::optional<GroundPowerStatus> FssEJet::GetGroundPowerStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kGpuStateLVar))
    {
        return GroundPowerStatus::Unknown;
    }

    const double state = variableGateway_->GetLVar(kGpuStateLVar, kGpuStateInactive);

    if (state == kGpuStateFeeding)
    {
        return GroundPowerStatus::Connected;
    }

    if (state == kGpuStateInactive || state == kGpuStateHidden)
    {
        return GroundPowerStatus::Disconnected;
    }

    return GroundPowerStatus::Unknown;
}

void FssEJet::SetGroundPower(const bool on)
{
    gpuRule_.Request(on);
}

bool FssEJet::SetChocks(const bool placed)
{
    const double value = placed ? kChocksPlaced : kChocksStowed;
    for (const char* lVar : kChocksLVars)
    {
        variableGateway_->SetLVar(lVar, value);
    }

    return true;
}

void FssEJet::ClearOwnGroundEquipment()
{
    for (const char* lVar : kOwnGroundEquipmentLVars)
    {
        variableGateway_->SetLVar(lVar, kEquipmentStowed);
    }
}

bool FssEJet::IsPowered() const
{
    return variableGateway_->GetLVar(kElecPwrAcAvailLVar, 0.0) > 0.0;
}

bool FssEJet::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool FssEJet::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool FssEJet::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool FssEJet::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || AreChocksSet();
}

bool FssEJet::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkBrakeLeverLVar, 0.0) > 0.0;
}

bool FssEJet::IsBeaconOn() const
{
    return variableGateway_->GetLVar(kBeaconSwitchLVar, 0.0) > 0.0;
}

bool FssEJet::AreChocksSet() const
{
    for (const char* lVar : kChocksLVars)
    {
        if (variableGateway_->GetLVar(lVar, 0.0) > 0.0)
        {
            return true;
        }
    }

    return false;
}

namespace
{
    std::unique_ptr<Aircraft> MakeFssEJet(const AircraftContext& context, const AircraftIdentity& identity,
                                          const char* name)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "Freighter");

        return std::make_unique<FssEJet>(context.variableGateway, context.status, name, cargo);
    }

    std::unique_ptr<Aircraft> CreateFssE190(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return MakeFssEJet(context, identity, FssEJet::kNameE190);
    }

    std::unique_ptr<Aircraft> CreateFssE195(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return MakeFssEJet(context, identity, FssEJet::kNameE195);
    }

    const AircraftDescriptor kFssE190Descriptor{
        FssEJet::kNameE190,
        {
            {MatchField::Title, MatchOp::StartsWith, "FSS Embraer E190"}
        },
        &CreateFssE190, "fss-e190", "E190", RefuelBy::Client
    };

    const AircraftDescriptor kFssE195Descriptor{
        FssEJet::kNameE195,
        {
            {MatchField::Title, MatchOp::StartsWith, "FSS Embraer E195"}
        },
        &CreateFssE195, "fss-e195", "E195", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kFssE190Registration{kFssE190Descriptor};
    [[maybe_unused]] const AircraftRegistration kFssE195Registration{kFssE195Descriptor};
}
