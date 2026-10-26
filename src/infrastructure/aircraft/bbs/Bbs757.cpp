#include "Bbs757.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <QtCore/QString>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../logging/LogMacros.h"
#include "../../probe/ProbeLog.h"
#include "../../simvars/VariableGateway.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/model/FlightPlan.h"
#include "../../../domain/support/Weight.h"

using namespace simvars;

namespace
{
    constexpr double kRecommendedFuelRateKgs = 27.5;

    constexpr auto kPoundsUnit = "pounds";
    constexpr auto kGallonsUnit = "gallons";
    constexpr auto kVoltsUnit = "volts";
    constexpr auto kKnotsUnit = "Knots";

    constexpr auto kSetTotalFuelLVar = "BBS_EFBX_SET_TOTAL_FUEL_LBS";
    constexpr auto kSetPassengerWeightLVar = "BBS_EFBX_SET_PAX_WEIGHT_LBS";
    constexpr auto kSetCargoForwardLVar = "BBS_EFBX_SET_CARGO_FWD_LBS";
    constexpr auto kSetCargoAftLVar = "BBS_EFBX_SET_CARGO_AFT_LBS";

    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";
    constexpr auto kCentreTankCapacity = "FUELSYSTEM TANK CAPACITY:1";
    constexpr auto kLeftTankCapacity = "FUELSYSTEM TANK CAPACITY:2";
    constexpr auto kRightTankCapacity = "FUELSYSTEM TANK CAPACITY:3";
    constexpr double kCentreUnusableGallons = 12.0;
    constexpr double kWingsUnusableGallons = 8.0;

    constexpr double kPassengerWeightLb = 185.0;
    constexpr double kForwardHoldCapacityLb = 10300.0;
    constexpr double kAftHoldCapacityLb = 16300.0;
    constexpr double kHoldCapacitiesLb = kForwardHoldCapacityLb + kAftHoldCapacityLb;

    constexpr auto kSimPayloadStationPrefix = "PAYLOAD STATION WEIGHT:";
    constexpr std::array kCrewStations = {1, 2};
    constexpr std::array kPassengerStations = {3, 4};
    constexpr std::array kCargoStations = {5, 6};

    constexpr auto kSimGroundVelocity = "GROUND VELOCITY";
    constexpr double kVendorStoppedBelowKnots = 1.0;

    constexpr std::array kPowerBusVoltages = {"ELECTRICAL MAIN BUS VOLTAGE:9", "ELECTRICAL MAIN BUS VOLTAGE:11"};

    constexpr std::array kDoorPositionLVars = {
        "B757_DOORS_PAX_L1", "B757_DOORS_PAX_L2", "B757_DOORS_PAX_L3",
        "B757_DOORS_PAX_R1", "B757_DOORS_PAX_R2", "B757_DOORS_PAX_R3",
        "B757_DOORS_CARGO_FWD", "B757_DOORS_CARGO_AFT",
    };
    constexpr auto kL1DoorSwitchLVar = "L1_PAX_DOOR_STATE_SWITCH";
    constexpr auto kL2DoorSwitchLVar = "L2_PAX_DOOR_STATE_SWITCH";
    constexpr auto kL3DoorSwitchLVar = "L3_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR1DoorSwitchLVar = "R1_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR2DoorSwitchLVar = "R2_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR3DoorSwitchLVar = "R3_PAX_DOOR_STATE_SWITCH";
    constexpr auto kForwardCargoDoorSwitchLVar = "FWD_CARGO_DOOR_STATE_SWITCH";
    constexpr auto kAftCargoDoorSwitchLVar = "AFT_CARGO_DOOR_STATE_SWITCH";
    constexpr double kDoorSwitchClosed = 0.0;

    constexpr auto kGpuCartLVar = "BBS_757_GPU_CART_VISIBLE";
    constexpr auto kParkBrakeLVar = "B757_FC_PARK_BRAKE";
    constexpr auto kChocksLVar = "BBS757_vis_chocks";
    constexpr auto kBeaconLVar = "B757_OVHD_BEACON_LIGHT";
    constexpr std::array kPilotOnlyGroundEquipmentLVars = {kChocksLVar, kGpuCartLVar};

    constexpr int kEngineCount = 2;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kCaptainPttLVar = "B757_PED_AUDIO_1_PTT_SWITCH";
    constexpr auto kFirstOfficerPttLVar = "B757_PED_AUDIO_2_PTT_SWITCH";
    constexpr auto kSmartSwitchControl = "PTT";
    constexpr double kPttAtRest = 0.0;

    constexpr int kProbeDecimals = 2;

    struct FuelCapacity
    {
        double wingsUsableGallons;
        double usableGallons;
    };

    std::optional<double> PoundsPerGallon(VariableGateway& variables)
    {
        if (!variables.HasReceivedAVar(kSimFuelWeightPerGallon, kPoundsUnit))
        {
            return std::nullopt;
        }

        return variables.GetAVar(kSimFuelWeightPerGallon, kPoundsUnit, 0.0);
    }

    std::optional<FuelCapacity> UsableFuelCapacity(VariableGateway& variables)
    {
        for (const char* tank : {kCentreTankCapacity, kLeftTankCapacity, kRightTankCapacity})
        {
            if (!variables.HasReceivedAVar(tank, kGallonsUnit))
            {
                return std::nullopt;
            }
        }

        const double centreGallons = variables.GetAVar(kCentreTankCapacity, kGallonsUnit, 0.0);
        const double wingsGallons = variables.GetAVar(kLeftTankCapacity, kGallonsUnit, 0.0)
            + variables.GetAVar(kRightTankCapacity, kGallonsUnit, 0.0);
        const double wingsUsableGallons = wingsGallons - kWingsUnusableGallons;

        return FuelCapacity{
            .wingsUsableGallons = wingsUsableGallons,
            .usableGallons = wingsUsableGallons + centreGallons - kCentreUnusableGallons,
        };
    }

    double GrossFuelLb(const double usableGallons, const FuelCapacity& capacity, const double poundsPerGallon)
    {
        if (usableGallons <= 0.0)
        {
            return 0.0;
        }

        const double unusableGallons = usableGallons <= capacity.wingsUsableGallons
                                           ? kWingsUnusableGallons
                                           : kWingsUnusableGallons + kCentreUnusableGallons;

        return (usableGallons + unusableGallons) * poundsPerGallon;
    }

    double PassengerShare(const int passengers, const double plannedPayloadKg)
    {
        const double plannedPayloadLb = weight::KgToLb(plannedPayloadKg);
        if (plannedPayloadLb <= 0.0)
        {
            return passengers > 0 ? 1.0 : 0.0;
        }

        return std::min(1.0, passengers * kPassengerWeightLb / plannedPayloadLb);
    }

    std::string PayloadStationVar(const int station)
    {
        return kSimPayloadStationPrefix + std::to_string(station);
    }

    bool IsStopped(VariableGateway& variables)
    {
        return variables.GetAVar(kSimGroundVelocity, kKnotsUnit, kVendorStoppedBelowKnots) < kVendorStoppedBelowKnots;
    }

    std::optional<double> StationsLb(VariableGateway& variables, const std::span<const int> stations)
    {
        double totalLb = 0.0;
        for (const int stationNumber : stations)
        {
            const std::string station = PayloadStationVar(stationNumber);
            if (!variables.HasReceivedAVar(station, kPoundsUnit))
            {
                return std::nullopt;
            }

            totalLb += variables.GetAVar(station, kPoundsUnit, 0.0);
        }

        return totalLb;
    }

    std::optional<double> CrewOnBoardLb(VariableGateway& variables)
    {
        if (!IsStopped(variables))
        {
            return std::nullopt;
        }

        return StationsLb(variables, kCrewStations);
    }
}

Bbs757::Bbs757(VariableGateway* variableGateway, const AutomationStatus* status)
    : variableGateway_(variableGateway),
      status_(status),
      smartSwitch_(*variableGateway, {kCaptainPttLVar, kFirstOfficerPttLVar},
                   [](double, const double max)
                   {
                       return max > kPttAtRest;
                   }),
      doors_(variableGateway),
      doorRule_(doors_),
      rules_{&doorRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: %s", kName);
}

bool Bbs757::IsCargoVariant() const
{
    return false;
}

void Bbs757::Observe()
{
    doors_.Observe();
}

const std::vector<AircraftRule*>& Bbs757::Rules() const
{
    return rules_;
}

MemoryBag Bbs757::TurnaroundMemory() const
{
    MemoryBag memory;
    doors_.AppendMemory(memory);

    return memory;
}

const char* Bbs757::DoorSwitch(const GsxDoor door)
{
    switch (door)
    {
    case GsxDoor::FwdPax:
        return kL1DoorSwitchLVar;
    case GsxDoor::MidPax:
        return kL2DoorSwitchLVar;
    case GsxDoor::AftPax:
        return kL3DoorSwitchLVar;
    case GsxDoor::FwdCatering:
        return kR1DoorSwitchLVar;
    case GsxDoor::AftCatering:
        return kR3DoorSwitchLVar;
    case GsxDoor::FwdCargo:
        return kForwardCargoDoorSwitchLVar;
    default:
        return kAftCargoDoorSwitchLVar;
    }
}

bool Bbs757::IsReachable() const
{
    return variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit)
        && variableGateway_->HasReceivedAVar(kSimFuelTotalKg, kKgUnit)
        && PoundsPerGallon(*variableGateway_).has_value()
        && UsableFuelCapacity(*variableGateway_).has_value()
        && variableGateway_->HasReceivedLVar(kSetTotalFuelLVar);
}

void Bbs757::OnTurnaroundStarted()
{
    resumedPlan_.reset();
    lastWritten_.clear();
}

void Bbs757::OnTurnaroundResumed(const TurnaroundFacts& facts, const MemoryBag& memory)
{
    lastWritten_.clear();
    doors_.RestoreMemory(memory, facts.gsxRestartedSinceSave);

    if (facts.plannedZfwKg > facts.emptyZfwKg)
    {
        resumedPlan_ = ResumedPlan{
            .aboveEmptyKg = facts.plannedZfwKg - facts.emptyZfwKg,
            .passengers = facts.plannedPassengers,
        };
    }
}

bool Bbs757::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready
        && status_->plannedPayloadKg.has_value()
        && variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit)
        && CrewOnBoardLb(*variableGateway_).has_value();
}

double Bbs757::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double Bbs757::GetPlannedZfwKg() const
{
    return GetEmptyZfwKg() + GetCrewOnBoardKg() + status_->plannedPayloadKg.value_or(0.0);
}

double Bbs757::GetPlannedOperatingEmptyKg() const
{
    return status_->plannedOperatingEmptyKg;
}

double Bbs757::GetCrewOnBoardKg() const
{
    return weight::LbToKg(CrewOnBoardLb(*variableGateway_).value_or(0.0));
}

int Bbs757::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double Bbs757::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double Bbs757::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double Bbs757::GetFuelCapacityKg() const
{
    const std::optional<double> poundsPerGallon = PoundsPerGallon(*variableGateway_);
    const std::optional<FuelCapacity> capacity = UsableFuelCapacity(*variableGateway_);
    if (!poundsPerGallon.has_value() || !capacity.has_value())
    {
        return 0.0;
    }

    return weight::LbToKg(capacity->usableGallons * *poundsPerGallon);
}

void Bbs757::SetCurrentFuelKg(const double fuelKg)
{
    const std::optional<double> poundsPerGallon = PoundsPerGallon(*variableGateway_);
    const std::optional<FuelCapacity> capacity = UsableFuelCapacity(*variableGateway_);
    if (!poundsPerGallon.has_value() || *poundsPerGallon <= 0.0
        || !capacity.has_value() || capacity->usableGallons <= 0.0)
    {
        return;
    }

    const double usableGallons = std::min(weight::KgToLb(fuelKg) / *poundsPerGallon, capacity->usableGallons);

    WriteWhenChanged(kSetTotalFuelLVar, GrossFuelLb(usableGallons, *capacity, *poundsPerGallon));
}

double Bbs757::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

void Bbs757::SetCurrentZfwKg(const double zfwKg)
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return;
    }

    const PlannedLoad plan = PlanAboard();
    const double plannedPayloadLb = weight::KgToLb(plan.payloadKg);
    const double payloadLb = std::clamp(
        weight::KgToLb(zfwKg - GetEmptyZfwKg() - GetCrewOnBoardKg()), 0.0, plannedPayloadLb);
    const double passengersLb = payloadLb * plan.passengerShare;
    const double cargoLb = std::min(payloadLb - passengersLb, kHoldCapacitiesLb);

    WriteWhenChanged(kSetPassengerWeightLVar, passengersLb);
    WriteWhenChanged(kSetCargoForwardLVar, cargoLb * kForwardHoldCapacityLb / kHoldCapacitiesLb);
    WriteWhenChanged(kSetCargoAftLVar, cargoLb * kAftHoldCapacityLb / kHoldCapacitiesLb);
}

Bbs757::PlannedLoad Bbs757::PlanAboard() const
{
    if (status_->plannedPayloadKg.has_value())
    {
        const double payloadKg = *status_->plannedPayloadKg;

        return {.payloadKg = payloadKg, .passengerShare = PassengerShare(status_->plannedPassengers, payloadKg)};
    }

    if (resumedPlan_.has_value())
    {
        const double payloadKg = std::max(0.0, resumedPlan_->aboveEmptyKg - GetCrewOnBoardKg());

        return {.payloadKg = payloadKg, .passengerShare = PassengerShare(resumedPlan_->passengers, payloadKg)};
    }

    return PlanFromLoadOnBoard();
}

Bbs757::PlannedLoad Bbs757::PlanFromLoadOnBoard() const
{
    const std::optional<double> passengersLb = StationsLb(*variableGateway_, kPassengerStations);
    const std::optional<double> cargoLb = StationsLb(*variableGateway_, kCargoStations);
    if (!passengersLb.has_value() || !cargoLb.has_value())
    {
        return {.payloadKg = 0.0, .passengerShare = 0.0};
    }

    const double payloadLb = *passengersLb + *cargoLb;

    return {
        .payloadKg = weight::LbToKg(payloadLb),
        .passengerShare = payloadLb > 0.0 ? *passengersLb / payloadLb : 0.0,
    };
}

void Bbs757::WriteWhenChanged(const char* lVar, const double value)
{
    if (!variableGateway_->HasReceivedLVar(lVar))
    {
        return;
    }

    const auto last = lastWritten_.find(lVar);
    if (last != lastWritten_.end() && last->second == value)
    {
        return;
    }

    lastWritten_[lVar] = value;

    probe::Line(probe::Channel::Writes,
                QStringLiteral("write %1=%2").arg(QLatin1String(lVar)).arg(value, 0, 'f', kProbeDecimals));
    variableGateway_->SetLVar(lVar, value);
}

bool Bbs757::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

std::optional<GroundPowerStatus> Bbs757::GetGroundPowerStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kGpuCartLVar))
    {
        return GroundPowerStatus::Unknown;
    }

    if (variableGateway_->GetLVar(kGpuCartLVar, 0.0) > 0.0)
    {
        return GroundPowerStatus::Connected;
    }

    return std::nullopt;
}

bool Bbs757::IsOwnGroundEquipmentAttached() const
{
    return std::ranges::any_of(kPilotOnlyGroundEquipmentLVars, [this](const char* lVar)
    {
        return !variableGateway_->HasReceivedLVar(lVar) || variableGateway_->GetLVar(lVar, 0.0) > 0.0;
    });
}

void Bbs757::CloseAllDoors()
{
    probe::Line(probe::Channel::Writes, QStringLiteral("write doors switches=0"));

    doors_.CloseAll([this](const GsxDoor door, bool)
    {
        variableGateway_->SetLVar(DoorSwitch(door), kDoorSwitchClosed);
    });

    variableGateway_->SetLVar(kR2DoorSwitchLVar, kDoorSwitchClosed);
}

void Bbs757::HoldDoorsClosed(const bool hold)
{
    doors_.HoldClosedForDeparture(hold);
}

void Bbs757::HoldPassengerDoorsClosed(const bool hold)
{
    doors_.HoldPassengerDoorsClosed(hold);
}

bool Bbs757::IsPowered() const
{
    return std::ranges::any_of(kPowerBusVoltages, [this](const char* bus)
    {
        return variableGateway_->GetAVar(bus, kVoltsUnit, 0.0) > 0.0;
    });
}

DoorStatus Bbs757::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* doorLVar : kDoorPositionLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, doorLVar));
    }

    return status;
}

bool Bbs757::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool Bbs757::IsReadyToDeboard() const
{
    return variableGateway_->HasReceivedLVar(kBeaconLVar) && !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool Bbs757::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool Bbs757::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || AreChocksSet();
}

bool Bbs757::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkBrakeLVar, 0.0) > 0.0;
}

bool Bbs757::IsBeaconOn() const
{
    return variableGateway_->GetLVar(kBeaconLVar, 0.0) > 0.0;
}

bool Bbs757::AreChocksSet() const
{
    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateBbs757(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<Bbs757>(context.variableGateway, context.status);
    }

    const AircraftDescriptor kBbs757Descriptor{
        Bbs757::kName,
        {
            {MatchField::Title, MatchOp::StartsWith, "BlueBird 757-200"},
        },
        &CreateBbs757, "bbs-757-200", "B752", RefuelBy::Client, SmartSwitchCue{kSmartSwitchControl, "", SmartSwitchMove::Press}, kRecommendedFuelRateKgs,
    };

    [[maybe_unused]] const AircraftRegistration kBbs757Registration{kBbs757Descriptor};
}
