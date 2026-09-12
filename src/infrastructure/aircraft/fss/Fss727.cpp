#include "Fss727.h"

#include "../../simvars/SimVars.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
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

    constexpr auto kAcPowerAvailableLVar = "FSS_B727_FE_ELEC_AC_PWR_AVAIL";

    constexpr int kEngineCount = 3;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kParkBrakeLeverLVar = "FSS_B727_PDSTL_PARK_BRAKE_LEVER";
    constexpr auto kChocksLVar = "FSS_B727_EFB_CHOCKS_VISIBLE";

    constexpr auto kPhoneLVar = "FSS_B727_PDSTL_PHONE_PICK_UP";
    constexpr double kPhoneAtRest = 0.0;

    constexpr auto kPercentOver100Unit = "percent over 100";
    constexpr double kDoorPointClosedAtMost = 0.05;
    constexpr double kDoorPointOpenAtLeast = 0.95;

    constexpr std::array kFreighterDoorPoints = {
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:0", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:1", doors::kMainDeckDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:2", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:3", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:4", doors::kPaxDoorMovingLimitTicks}
    };

    constexpr std::array kPassengerDoorPoints = {
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:0", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:1", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:2", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:3", doors::kPaxDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:4", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:5", doors::kCargoDoorMovingLimitTicks},
        Fss727DoorPoint{"INTERACTIVE POINT OPEN:6", doors::kPaxDoorMovingLimitTicks}
    };

    std::span<const Fss727DoorPoint> DoorPointsFor(const bool cargoVariant)
    {
        if (cargoVariant)
        {
            return kFreighterDoorPoints;
        }

        return kPassengerDoorPoints;
    }

    bool IsTravelling(const double position)
    {
        return position > kDoorPointClosedAtMost && position < kDoorPointOpenAtLeast;
    }
}

Fss727::Fss727(VariableGateway* variableGateway, const AutomationStatus* status, const char* name,
               const bool cargoVariant)
    : variableGateway_(variableGateway),
      status_(status),
      cargoVariant_(cargoVariant),
      smartSwitch_(*variableGateway, {kPhoneLVar},
                   [](double, const double max)
                   {
                       return max > kPhoneAtRest;
                   }),
      doorPoints_(DoorPointsFor(cargoVariant)),
      movingTicks_(doorPoints_.size(), 0),
      automodeRule_(*variableGateway),
      rules_{&automodeRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: %s", name);
}

bool Fss727::IsCargoVariant() const
{
    return cargoVariant_;
}

const std::vector<AircraftRule*>& Fss727::Rules() const
{
    return rules_;
}

void Fss727::Observe()
{
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
    if (!variableGateway_->HasReceivedAVar(kSimFuelWeightPerGallon, kPoundsUnit))
    {
        return 0.0;
    }

    double capacityGallons = 0.0;
    for (const char* tankCapacity : kTankCapacities)
    {
        if (!variableGateway_->HasReceivedAVar(tankCapacity, kGallonsUnit))
        {
            return 0.0;
        }

        capacityGallons += variableGateway_->GetAVar(tankCapacity, kGallonsUnit, 0.0);
    }

    const double poundsPerGallon = variableGateway_->GetAVar(kSimFuelWeightPerGallon, kPoundsUnit, 0.0);

    return weight::LbToKg(capacityGallons * poundsPerGallon);
}

double Fss727::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

bool Fss727::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
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
        return std::make_unique<Fss727>(context.variableGateway, context.status, Fss727::kName200F, true);
    }

    std::unique_ptr<Aircraft> CreateFss727200ReFreighter(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<Fss727>(context.variableGateway, context.status, Fss727::kName200ReFreighter, true);
    }

    std::unique_ptr<Aircraft> CreateFss727200RePassenger(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<Fss727>(context.variableGateway, context.status, Fss727::kName200RePassenger, false);
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

    const AircraftDescriptor kFss727200RePassengerDescriptor{
        Fss727::kName200RePassenger,
        {
            {MatchField::Title, MatchOp::StartsWith, "Boeing 727-200RE Passenger"},
            {MatchField::Title, MatchOp::StartsWith, "Boeing 727-200RE Super 27 Passenger"}
        },
        &CreateFss727200RePassenger, "fss-727-200rep", "R72P", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kFss727200FRegistration{kFss727200FDescriptor};
    [[maybe_unused]] const AircraftRegistration kFss727200ReFreighterRegistration{kFss727200ReFreighterDescriptor};
    [[maybe_unused]] const AircraftRegistration kFss727200RePassengerRegistration{kFss727200RePassengerDescriptor};
}
