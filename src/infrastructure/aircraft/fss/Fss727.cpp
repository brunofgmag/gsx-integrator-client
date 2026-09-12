#include "Fss727.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../logging/LogMacros.h"
#include "../../simvars/VariableGateway.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/model/FlightPlan.h"
#include "../../../domain/support/Weight.h"

using namespace simvars;

namespace
{
    constexpr auto kPoundsUnit = "pounds";
    constexpr auto kGallonsUnit = "gallons";

    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";
    constexpr std::array kTankCapacities = {
        "FUELSYSTEM TANK CAPACITY:1", "FUELSYSTEM TANK CAPACITY:2", "FUELSYSTEM TANK CAPACITY:3"
    };
    constexpr std::array kTankLevels = {
        "FUELSYSTEM TANK LEVEL:1", "FUELSYSTEM TANK LEVEL:2", "FUELSYSTEM TANK LEVEL:3"
    };

    constexpr auto kSimPayloadStationPrefix = "PAYLOAD STATION WEIGHT:";
    constexpr int kFirstCargoStation = 4;
    constexpr std::array kCargoStationCapacitiesLb = {
        0.0, 0.0, 5671.5, 5671.5, 6301.5, 6301.5, 7500.0, 7500.0, 8327.0,
        7769.0, 7769.0, 4000.0, 2335.55, 3805.0, 4557.0, 3653.0, 3649.0, 4026.0
    };
    constexpr std::array kCrewStations = {1, 2, 3};

    constexpr auto kAcPowerAvailableLVar = "FSS_B727_FE_ELEC_AC_PWR_AVAIL";
    constexpr auto kGpuAvailableLVar = "FSS_B727_GPU_AVAIL";

    constexpr int kEngineCount = 3;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kParkBrakeLeverLVar = "FSS_B727_PDSTL_PARK_BRAKE_LEVER";
    constexpr auto kChocksLVar = "FSS_B727_EFB_CHOCKS_VISIBLE";
    constexpr auto kConesLVar = "FSS_B727_EFB_CONES_VISIBLE";
    constexpr auto kEngineCoversLVar = "FSS_B727_EFB_COVER_ENGINE_VISIBLE";
    constexpr auto kBoardingStairLVar = "FSS_B727_EFB_BOARDING_STAIR";

    constexpr std::array kOwnGroundEquipmentLVars = {
        kChocksLVar, kConesLVar, kEngineCoversLVar, kBoardingStairLVar
    };

    constexpr double kEquipmentStowed = 0.0;
    constexpr double kEquipmentPlaced = 1.0;

    constexpr auto kPhoneLVar = "FSS_B727_PDSTL_PHONE_PICK_UP";
    constexpr double kPhoneAtRest = 0.0;

    constexpr auto kPercentOver100Unit = "percent over 100";
    constexpr std::size_t kMainDeckPoint = 1;
    constexpr std::array kGoalsClosedWithEveryDoor = {
        "INTERACTIVE POINT GOAL:0", "INTERACTIVE POINT GOAL:2", "INTERACTIVE POINT GOAL:3"
    };
    constexpr double kDoorGoalClosed = 0.0;
    constexpr double kDoorPointClosedAtMost = 0.05;
    constexpr double kDoorPointOpenAtLeast = 0.95;

    constexpr std::array kFreighterDoorPoints = {
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:0", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:1", doors::kMainDeckDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:2", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:3", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:4", doors::kPaxDoorMovingLimitTicks}
    };

    bool IsTravelling(const double position)
    {
        return position > kDoorPointClosedAtMost && position < kDoorPointOpenAtLeast;
    }

    std::optional<double> PoundsPerGallon(VariableGateway& variables)
    {
        if (!variables.HasReceivedAVar(kSimFuelWeightPerGallon, kPoundsUnit))
        {
            return std::nullopt;
        }

        return variables.GetAVar(kSimFuelWeightPerGallon, kPoundsUnit, 0.0);
    }

    std::optional<double> TankCapacityGallons(VariableGateway& variables)
    {
        double capacityGallons = 0.0;
        for (const char* tankCapacity : kTankCapacities)
        {
            if (!variables.HasReceivedAVar(tankCapacity, kGallonsUnit))
            {
                return std::nullopt;
            }

            capacityGallons += variables.GetAVar(tankCapacity, kGallonsUnit, 0.0);
        }

        return capacityGallons;
    }

    std::string PayloadStationVar(const int station)
    {
        return kSimPayloadStationPrefix + std::to_string(station);
    }

    double CrewOnBoardLb(VariableGateway& variables)
    {
        double crewLb = 0.0;
        for (const int crewStation : kCrewStations)
        {
            const std::string station = PayloadStationVar(crewStation);
            if (!variables.HasReceivedAVar(station, kPoundsUnit))
            {
                return 0.0;
            }

            crewLb += variables.GetAVar(station, kPoundsUnit, 0.0);
        }

        return crewLb;
    }
}

Fss727::Fss727(VariableGateway* variableGateway, const AutomationStatus* status, const char* name,
               const GsxGateway* gsxGateway)
    : variableGateway_(variableGateway),
      status_(status),
      smartSwitch_(*variableGateway, {kPhoneLVar},
                   [](double, const double max)
                   {
                       return max > kPhoneAtRest;
                   }),
      doorPoints_(kFreighterDoorPoints),
      movingTicks_(doorPoints_.size(), 0),
      doors_(variableGateway),
      automodeRule_(*variableGateway),
      groundPowerRule_(gsxGateway),
      frontEntryRule_(*variableGateway, *this, doors_),
      mainDeckRule_(*this, gsxGateway),
      rules_{&automodeRule_, &groundPowerRule_, &frontEntryRule_, &mainDeckRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: %s", name);
}

bool Fss727::IsCargoVariant() const
{
    return true;
}

const std::vector<AircraftRule*>& Fss727::Rules() const
{
    return rules_;
}

void Fss727::Observe()
{
    doors_.Observe();

    for (std::size_t point = 0; point < doorPoints_.size(); ++point)
    {
        const std::optional<double> position = DoorPointPosition(point);
        int& ticks = movingTicks_[point];
        ticks = position.has_value() && IsTravelling(*position) ? ticks + 1 : 0;
    }
}

bool Fss727::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
}

double Fss727::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double Fss727::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int Fss727::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double Fss727::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double Fss727::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double Fss727::GetFuelCapacityKg() const
{
    const std::optional<double> poundsPerGallon = PoundsPerGallon(*variableGateway_);
    const std::optional<double> capacityGallons = TankCapacityGallons(*variableGateway_);
    if (!poundsPerGallon.has_value() || !capacityGallons.has_value())
    {
        return 0.0;
    }

    return weight::LbToKg(*capacityGallons * *poundsPerGallon);
}

void Fss727::SetCurrentFuelKg(const double fuelKg)
{
    const std::optional<double> poundsPerGallon = PoundsPerGallon(*variableGateway_);
    const std::optional<double> capacityGallons = TankCapacityGallons(*variableGateway_);
    if (!poundsPerGallon.has_value() || *poundsPerGallon <= 0.0
        || !capacityGallons.has_value() || *capacityGallons <= 0.0
        || fuelKg == lastFuelKg_)
    {
        return;
    }

    lastFuelKg_ = fuelKg;

    const double level = std::clamp(weight::KgToLb(fuelKg) / *poundsPerGallon / *capacityGallons, 0.0, 1.0);

    for (const char* tankLevel : kTankLevels)
    {
        variableGateway_->SetAVar(tankLevel, kPercentOver100Unit, level);
    }
}

double Fss727::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

void Fss727::SetCurrentZfwKg(const double zfwKg)
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit) || zfwKg == lastZfwKg_)
    {
        return;
    }

    lastZfwKg_ = zfwKg;

    const double cargoLb = std::max(
        weight::KgToLb(zfwKg - GetEmptyZfwKg()) - CrewOnBoardLb(*variableGateway_), 0.0);

    double capacitiesLb = 0.0;
    for (const double capacityLb : kCargoStationCapacitiesLb)
    {
        capacitiesLb += capacityLb;
    }

    for (std::size_t station = 0; station < kCargoStationCapacitiesLb.size(); ++station)
    {
        const double stationLb = cargoLb * kCargoStationCapacitiesLb[station] / capacitiesLb;

        variableGateway_->SetAVar(
            PayloadStationVar(static_cast<int>(station) + kFirstCargoStation), kPoundsUnit, stationLb);
    }
}

bool Fss727::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

void Fss727::ClearOwnGroundEquipment()
{
    for (const char* lVar : kOwnGroundEquipmentLVars)
    {
        variableGateway_->SetLVar(lVar, kEquipmentStowed);
    }
}

bool Fss727::SetChocks(const bool placed)
{
    variableGateway_->SetLVar(kChocksLVar, placed ? kEquipmentPlaced : kEquipmentStowed);

    return true;
}

std::optional<GroundPowerStatus> Fss727::GetGroundPowerStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kGpuAvailableLVar))
    {
        return GroundPowerStatus::Unknown;
    }

    return variableGateway_->GetLVar(kGpuAvailableLVar, 0.0) > 0.0
               ? GroundPowerStatus::Connected
               : GroundPowerStatus::Disconnected;
}

void Fss727::CloseAllDoors()
{
    for (const char* goal : kGoalsClosedWithEveryDoor)
    {
        variableGateway_->SetAVar(goal, kPercentOver100Unit, kDoorGoalClosed);
    }

    ++mainDeckCloseRequests_;
}

void Fss727::HoldDoorsClosed(const bool hold)
{
    heldForDeparture_ = hold;
    doors_.HoldClosedForDeparture(hold);
}

bool Fss727::IsHeldForDeparture() const
{
    return heldForDeparture_;
}

int Fss727::MainDeckCloseRequests() const
{
    return mainDeckCloseRequests_;
}

std::optional<bool> Fss727::IsMainDeckClosed() const
{
    const std::optional<double> position = DoorPointPosition(kMainDeckPoint);
    if (!position.has_value())
    {
        return std::nullopt;
    }

    return *position <= kDoorPointClosedAtMost;
}

bool Fss727::IsPowered() const
{
    return variableGateway_->GetLVar(kAcPowerAvailableLVar, 0.0) > 0.0;
}

DoorStatus Fss727::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (std::size_t point = 0; point < doorPoints_.size(); ++point)
    {
        status = doors::Combine(status, DoorOpenAt(point));
    }

    return status;
}

std::optional<double> Fss727::DoorPointPosition(const std::size_t point) const
{
    const char* position = doorPoints_[point].position;
    if (!variableGateway_->HasReceivedAVar(position, kPercentOver100Unit))
    {
        return std::nullopt;
    }

    return variableGateway_->GetAVar(position, kPercentOver100Unit, 0.0);
}

std::optional<bool> Fss727::DoorOpenAt(const std::size_t point) const
{
    const std::optional<double> position = DoorPointPosition(point);
    if (!position.has_value())
    {
        return std::nullopt;
    }

    if (!IsTravelling(*position))
    {
        return *position >= kDoorPointOpenAtLeast;
    }

    return movingTicks_[point] >= doorPoints_[point].movingLimitTicks ? std::optional{true} : std::nullopt;
}

bool Fss727::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool Fss727::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool Fss727::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool Fss727::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || AreChocksSet();
}

bool Fss727::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkBrakeLeverLVar, 0.0) > 0.0;
}

bool Fss727::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

bool Fss727::AreChocksSet() const
{
    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateFss727200F(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<Fss727>(context.variableGateway, context.status, Fss727::kName200F,
                                       context.gsxGateway);
    }

    std::unique_ptr<Aircraft> CreateFss727200ReFreighter(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<Fss727>(context.variableGateway, context.status, Fss727::kName200ReFreighter,
                                       context.gsxGateway);
    }

    const AircraftDescriptor kFss727200FDescriptor{
        Fss727::kName200F,
        {
            {MatchField::Title, MatchOp::StartsWith, "Boeing 727-200F"},
            {MatchField::Title, MatchOp::StartsWith, "Boeing B727-200 Freighter"}
        },
        &CreateFss727200F, "fss-727-200f", "722F", RefuelBy::Client
    };

    const AircraftDescriptor kFss727200ReFreighterDescriptor{
        Fss727::kName200ReFreighter,
        {
            {MatchField::Title, MatchOp::StartsWith, "Boeing 727-200RE Freighter"},
            {MatchField::Title, MatchOp::StartsWith, "Boeing 727-200RE Super 27 Freighter"}
        },
        &CreateFss727200ReFreighter, "fss-727-200re", "R72F", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kFss727200FRegistration{kFss727200FDescriptor};
    [[maybe_unused]] const AircraftRegistration kFss727200ReFreighterRegistration{kFss727200ReFreighterDescriptor};
}
