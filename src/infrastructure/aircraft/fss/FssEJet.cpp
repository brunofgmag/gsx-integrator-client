#include "FssEJet.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <string>

#include "../AircraftRegistry.h"
#include "../DoorReading.h"
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

    constexpr auto kCallRampActiveLVar = "FSS_EXX_AUDIO_TEL_RAMP_ACTIVE";
    constexpr double kCallRampInactive = 0.0;

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

    struct DoorReadPoint
    {
        const char* openLVar;
        const char* movingLVar;
        const char* movingLVar2;
        int movingLimitTicks;
    };

    constexpr int kPaxDoorMovingLimitTicks = 14;

    constexpr std::array kPassengerDoorReadPoints = {
        DoorReadPoint{"FSS_EXX_DOOR_FWD_L_OPEN", "FSS_EXX_DOOR_FWD_L_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_AFT_L_OPEN", "FSS_EXX_DOOR_AFT_L_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_FWD_R_OPEN", "FSS_EXX_DOOR_FWD_R_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_AFT_R_OPEN", "FSS_EXX_DOOR_AFT_R_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_CARGO_FWD_OPEN", "FSS_EXX_DOOR_CARGO_FWD_MOVING", nullptr,
                      doors::kCargoDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_CARGO_AFT_OPEN", "FSS_EXX_DOOR_CARGO_AFT_MOVING", nullptr,
                      doors::kCargoDoorMovingLimitTicks}
    };

    constexpr std::array kCargoDoorReadPoints = {
        DoorReadPoint{"FSS_EXX_DOOR_FWD_L_OPEN", "FSS_EXX_DOOR_FWD_L_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_FWD_R_OPEN", "FSS_EXX_DOOR_FWD_R_MOVING", nullptr, kPaxDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_CARGO_FWD_OPEN", "FSS_EXX_DOOR_CARGO_FWD_MOVING", nullptr,
                      doors::kCargoDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_CARGO_AFT_OPEN", "FSS_EXX_DOOR_CARGO_AFT_MOVING", nullptr,
                      doors::kCargoDoorMovingLimitTicks},
        DoorReadPoint{"FSS_EXX_DOOR_CARGO_MAIN_OPEN", "FSS_EXX_DOOR_CARGO_MAIN_MOVING_UP",
                      "FSS_EXX_DOOR_CARGO_MAIN_MOVING_DN", doors::kMainDeckDoorMovingLimitTicks}
    };

    std::span<const DoorReadPoint> DoorReadPointsFor(const bool cargoVariant)
    {
        if (cargoVariant)
        {
            return kCargoDoorReadPoints;
        }

        return kPassengerDoorReadPoints;
    }

    bool IsDoorMoving(VariableGateway& variables, const DoorReadPoint& point)
    {
        if (variables.GetLVar(point.movingLVar, 0.0) > 0.0)
        {
            return true;
        }

        return point.movingLVar2 != nullptr && variables.GetLVar(point.movingLVar2, 0.0) > 0.0;
    }

    std::optional<bool> DoorOpenAt(VariableGateway& variables, const DoorReadPoint& point, const int movingTicks)
    {
        if (!variables.HasReceivedLVar(point.openLVar))
        {
            return std::nullopt;
        }

        if (!IsDoorMoving(variables, point))
        {
            return variables.GetLVar(point.openLVar, 0.0) > 0.0;
        }

        return movingTicks >= point.movingLimitTicks ? std::optional{true} : std::nullopt;
    }
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
                   }),
      doors_(variableGateway),
      doorMovingTicks_(DoorReadPointsFor(cargoVariant).size(), 0),
      automationRule_(*variableGateway),
      gpuRule_(*this),
      doorsRule_(*variableGateway, doors_, *this, cargoVariant),
      rules_{&automationRule_, &gpuRule_, &doorsRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: %s%s", name, cargoVariant ? " Freighter" : "");
}

bool FssEJet::IsCargoVariant() const
{
    return cargoVariant_;
}

void FssEJet::Observe()
{
    doors_.Observe();

    const std::span<const DoorReadPoint> points = DoorReadPointsFor(cargoVariant_);
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        doorMovingTicks_[i] = IsDoorMoving(*variableGateway_, points[i]) ? doorMovingTicks_[i] + 1 : 0;
    }
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
    const bool consumed = smartSwitch_.Consume();
    if (consumed)
    {
        variableGateway_->SetLVar(kCallRampActiveLVar, kCallRampInactive);
    }

    return consumed;
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

void FssEJet::CloseAllDoors()
{
    doorsRule_.RequestCloseAll();
}

void FssEJet::HoldDoorsClosed(const bool hold)
{
    doors_.HoldClosedForDeparture(hold);

    if (hold)
    {
        doorsRule_.RequestCloseAll();
    }
}

DoorStatus FssEJet::GetDoorStatus() const
{
    const std::span<const DoorReadPoint> points = DoorReadPointsFor(cargoVariant_);

    DoorStatus status = doors::kNoDoorsSeen;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        status = doors::Combine(status, DoorOpenAt(*variableGateway_, points[i], doorMovingTicks_[i]));
    }

    return status;
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
    return std::ranges::any_of(kChocksLVars, [this](const char* lVar)
    {
        return variableGateway_->GetLVar(lVar, 0.0) > 0.0;
    });
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
