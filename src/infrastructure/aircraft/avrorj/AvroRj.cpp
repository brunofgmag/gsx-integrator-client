#include "AvroRj.h"

#include "../../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <span>
#include <string>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../probe/ProbeLog.h"
#include "../../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kGallonsUnit = "Gallons";

    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";

    constexpr auto kLeftAuxFittedLVar = "OVHD_FUEL_L_aux_vis";
    constexpr auto kRightAuxFittedLVar = "OVHD_FUEL_R_aux_vis";

    struct FuelTank
    {
        const char* quantity;
        const char* capacity;
        const char* fitted = nullptr;
    };

    constexpr std::array kMainTanks = {
        FuelTank{"FUEL TANK LEFT MAIN QUANTITY", "FUEL TANK LEFT MAIN CAPACITY"},
        FuelTank{"FUEL TANK RIGHT MAIN QUANTITY", "FUEL TANK RIGHT MAIN CAPACITY"}
    };

    constexpr std::array kCenterTanks = {
        FuelTank{"FUEL TANK CENTER QUANTITY", "FUEL TANK CENTER CAPACITY"}
    };

    constexpr std::array kAuxTanks = {
        FuelTank{"FUEL TANK LEFT AUX QUANTITY", "FUEL TANK LEFT AUX CAPACITY", kLeftAuxFittedLVar},
        FuelTank{"FUEL TANK RIGHT AUX QUANTITY", "FUEL TANK RIGHT AUX CAPACITY", kRightAuxFittedLVar}
    };

    constexpr auto kPlannedBlockFuelLVar = "146_SimBrief_Block_Fuel";
    constexpr auto kPlannedZfwLVar = "146_SimBrief_ZFW";
    constexpr auto kPlannedPassengersLVar = "146_SimBrief_PaxQt";

    constexpr auto kAcBus2LVar = "JF_RJ_ELEC_AC_2";
    constexpr auto kAcBusEssLVar = "JF_RJ_ELEC_AC_ess";

    constexpr auto kChocksLVar = "EXT_Chocks";
    constexpr auto kParkBrakeAnnunciatorLVar = "C_ANNUNS_ParkBrake_il";

    constexpr auto kSmartSwitchLVar = "PED_FWD_L_Audio_RT";
    constexpr double kSmartSwitchNeutral = 1.0;

    constexpr auto kFwdPaxDoorLVar = "EXT_Door_pax_1L";
    constexpr auto kFwdServiceDoorLVar = "EXT_Door_pax_1R";
    constexpr auto kAftPaxDoorLVar = "EXT_Door_pax_2L";
    constexpr auto kAftServiceDoorLVar = "EXT_Door_pax_2R";
    constexpr auto kFwdCargoDoorLVar = "EXT_Door_cargo_fwd";
    constexpr auto kAftCargoDoorLVar = "EXT_Door_cargo_aft";
    constexpr auto kFuselageCargoDoorLVar = "EXT_Door_cargo_fuselage";

    constexpr std::array kDoorLVars = {
        kFwdPaxDoorLVar, kFwdServiceDoorLVar, kAftPaxDoorLVar, kAftServiceDoorLVar,
        kFwdCargoDoorLVar, kAftCargoDoorLVar, kFuselageCargoDoorLVar
    };

    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;

    constexpr double kJetwayUnavailable = 2.0;

    constexpr int kEngineCount = 4;

    bool IsTankFitted(VariableGateway& variables, const FuelTank& tank)
    {
        if (tank.fitted == nullptr)
        {
            return true;
        }

        return variables.HasReceivedLVar(tank.fitted) && variables.GetLVar(tank.fitted, 0.0) != 0.0;
    }

    bool HaveFittedFlagsArrived(VariableGateway& variables, const std::span<const FuelTank> tanks)
    {
        for (const auto& tank : tanks)
        {
            if (tank.fitted != nullptr && !variables.HasReceivedLVar(tank.fitted))
            {
                return false;
            }
        }

        return true;
    }

    double GroupCapacityGallons(VariableGateway& variables, const std::span<const FuelTank> tanks)
    {
        double capacity = 0.0;
        for (const auto& tank : tanks)
        {
            if (!IsTankFitted(variables, tank))
            {
                continue;
            }

            capacity += variables.GetAVar(tank.capacity, kGallonsUnit, 0.0);
        }

        return capacity;
    }

    double FillGroup(VariableGateway& variables, const std::span<const FuelTank> tanks,
                     const double remainingGallons)
    {
        const double capacity = GroupCapacityGallons(variables, tanks);
        const double share = std::min(std::max(remainingGallons, 0.0), capacity);

        for (const auto& tank : tanks)
        {
            if (!IsTankFitted(variables, tank))
            {
                continue;
            }

            const double tankCapacity = variables.GetAVar(tank.capacity, kGallonsUnit, 0.0);
            const double tankShare = capacity > 0.0 ? share * tankCapacity / capacity : 0.0;

            variables.SetAVar(tank.quantity, kGallonsUnit, tankShare);
        }

        return share;
    }
}

AvroRj::AvroRj(VariableGateway* variableGateway, const bool cargoVariant)
    : variableGateway_(variableGateway),
      cargoVariant_(cargoVariant),
      smartSwitch_(*variableGateway, {kSmartSwitchLVar},
                   [](double, const double max)
                   {
                       return max > kSmartSwitchNeutral;
                   }),
      doors_(variableGateway),
      doorRule_(*variableGateway, *this, doors_, airstair_),
      airstairRule_(*variableGateway, *this, doors_, airstair_),
      livenessRule_(*variableGateway, module_),
      rules_{&doorRule_, &airstairRule_, &livenessRule_}
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: JustFlight Avro RJ");
}

bool AvroRj::IsCargoVariant() const
{
    return cargoVariant_;
}

void AvroRj::Observe()
{
    doors_.Observe();
}

void AvroRj::HoldDoorsClosed(const bool hold)
{
    heldForDeparture_ = hold;
    doors_.HoldClosedForDeparture(hold);
}

bool AvroRj::IsHeldForDeparture() const
{
    return heldForDeparture_;
}

bool AvroRj::IsModuleMirroringFuel() const
{
    return module_.mirroringFuel;
}

bool AvroRj::SupportsStairsOrJetways() const
{
    return IsJetwayAvailable();
}

bool AvroRj::IsJetwayAvailable() const
{
    return doors_.VehicleState(gsx::lvars::kJetway, kJetwayUnavailable) != kJetwayUnavailable;
}

const std::vector<AircraftRule*>& AvroRj::Rules() const
{
    return rules_;
}

bool AvroRj::AreAirstairsSettled() const
{
    return airstair_.settled;
}

void AvroRj::OnLoadingStarted()
{
    LOG_INFO("Loading started: the aircraft loads itself from the plan imported into its EFB");
}

bool AvroRj::IsFlightPlanLoaded() const
{
    const bool passengersArrived = variableGateway_->GetLVar(kPlannedPassengersLVar, 0.0) > 0.0;

    return variableGateway_->GetLVar(kPlannedBlockFuelLVar, 0.0) > 0.0
        && variableGateway_->GetLVar(kPlannedZfwLVar, 0.0) > 0.0
        && (cargoVariant_ || passengersArrived);
}

double AvroRj::GetPlannedFuelKg() const
{
    return variableGateway_->GetLVar(kPlannedBlockFuelLVar, 0.0);
}

double AvroRj::GetPlannedZfwKg() const
{
    return variableGateway_->GetLVar(kPlannedZfwLVar, 0.0);
}

int AvroRj::GetPlannedPassengers() const
{
    return static_cast<int>(variableGateway_->GetLVar(kPlannedPassengersLVar, 0.0));
}

double AvroRj::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double AvroRj::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double AvroRj::KgPerGallon() const
{
    if (!variableGateway_->HasReceivedAVar(kSimFuelWeightPerGallon, kKgUnit))
    {
        return 0.0;
    }

    return variableGateway_->GetAVar(kSimFuelWeightPerGallon, kKgUnit, 0.0);
}

void AvroRj::SetCurrentFuelKg(const double fuelKg)
{
    const double kgPerGallon = KgPerGallon();
    if (kgPerGallon <= 0.0 || fuelKg == lastFuelKg_)
    {
        return;
    }

    const double capacityGallons = GroupCapacityGallons(*variableGateway_, kMainTanks)
        + GroupCapacityGallons(*variableGateway_, kCenterTanks)
        + GroupCapacityGallons(*variableGateway_, kAuxTanks);
    if (capacityGallons <= 0.0)
    {
        return;
    }

    lastFuelKg_ = fuelKg;

    double remainingGallons = std::min(std::max(fuelKg, 0.0) / kgPerGallon, capacityGallons);

    remainingGallons -= FillGroup(*variableGateway_, kMainTanks, remainingGallons);
    remainingGallons -= FillGroup(*variableGateway_, kCenterTanks, remainingGallons);
    FillGroup(*variableGateway_, kAuxTanks, remainingGallons);
}

double AvroRj::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

double AvroRj::GetFuelCapacityKg() const
{
    if (!HaveFittedFlagsArrived(*variableGateway_, kMainTanks)
        || !HaveFittedFlagsArrived(*variableGateway_, kCenterTanks)
        || !HaveFittedFlagsArrived(*variableGateway_, kAuxTanks))
    {
        return 0.0;
    }

    const double capacityGallons = GroupCapacityGallons(*variableGateway_, kMainTanks)
        + GroupCapacityGallons(*variableGateway_, kCenterTanks)
        + GroupCapacityGallons(*variableGateway_, kAuxTanks);

    return capacityGallons * KgPerGallon();
}

bool AvroRj::SetChocks(const bool placed)
{
    probe::Line(QStringLiteral("write chocks EXT_Chocks=%1").arg(placed ? 1 : 0));
    variableGateway_->SetLVar(kChocksLVar, placed ? 1.0 : 0.0);

    return true;
}

bool AvroRj::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool AvroRj::IsPowered() const
{
    return variableGateway_->GetLVar(kAcBus2LVar, 0.0) > 0.0
        || variableGateway_->GetLVar(kAcBusEssLVar, 0.0) > 0.0;
}

DoorStatus AvroRj::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* doorLVar : kDoorLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, doorLVar));
    }

    return status;
}

bool AvroRj::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool AvroRj::IsReadyToDeboard() const
{
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool AvroRj::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, 1.0, kEngineCount);
}

bool AvroRj::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || AreChocksSet();
}

bool AvroRj::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkBrakeAnnunciatorLVar, 0.0) > 0.0;
}

bool AvroRj::AreChocksSet() const
{
    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

bool AvroRj::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateAvroRj(const AircraftContext& context, const AircraftIdentity& identity)
    {
        const bool cargo = MatchText(identity.title, MatchOp::Contains, "RJ100 QT");

        return std::make_unique<AvroRj>(context.variableGateway, cargo);
    }

    const AircraftDescriptor kAvroRj70Descriptor{
        AvroRj::kNameRj70,
        {
            {MatchField::Title, MatchOp::StartsWith, "Just Flight RJ70"}
        },
        &CreateAvroRj, "justflight-rj70", "RJ70", RefuelBy::Client
    };

    const AircraftDescriptor kAvroRj85Descriptor{
        AvroRj::kNameRj85,
        {
            {MatchField::Title, MatchOp::StartsWith, "Just Flight RJ85"}
        },
        &CreateAvroRj, "justflight-rj85", "RJ85", RefuelBy::Client
    };

    const AircraftDescriptor kAvroRj100Descriptor{
        AvroRj::kNameRj100,
        {
            {MatchField::Title, MatchOp::StartsWith, "Just Flight RJ100"}
        },
        &CreateAvroRj, "justflight-rj100", "RJ1H", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kAvroRj70Registration{kAvroRj70Descriptor};
    [[maybe_unused]] const AircraftRegistration kAvroRj85Registration{kAvroRj85Descriptor};
    [[maybe_unused]] const AircraftRegistration kAvroRj100Registration{kAvroRj100Descriptor};
}
