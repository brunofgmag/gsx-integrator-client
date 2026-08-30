#include "AvroRj.h"

#include "../simvars/SimVars.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <span>
#include <string>
#include "AircraftRegistry.h"
#include "DoorReading.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kGallonsUnit = "Gallons";

    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";

    struct FuelTank
    {
        const char* quantity;
        const char* capacity;
    };

    constexpr std::array kMainTanks = {
        FuelTank{"FUEL TANK LEFT MAIN QUANTITY", "FUEL TANK LEFT MAIN CAPACITY"},
        FuelTank{"FUEL TANK RIGHT MAIN QUANTITY", "FUEL TANK RIGHT MAIN CAPACITY"}
    };

    constexpr std::array kCenterTanks = {
        FuelTank{"FUEL TANK CENTER QUANTITY", "FUEL TANK CENTER CAPACITY"}
    };

    constexpr std::array kAuxTanks = {
        FuelTank{"FUEL TANK LEFT AUX QUANTITY", "FUEL TANK LEFT AUX CAPACITY"},
        FuelTank{"FUEL TANK RIGHT AUX QUANTITY", "FUEL TANK RIGHT AUX CAPACITY"}
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

    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayUnavailable = 2.0;

    constexpr auto kStairArmClickspotLVar = "VC_Stairs_clickspot_LC";
    constexpr auto kStairExtendSwitchLVar = "CAB_CTRLS_Fwd_StairRetract";
    constexpr auto kStairAccumPressureLVar = "Stairs_accum_press";
    constexpr double kStairMinPressure = 500.0;
    constexpr double kStairSwitchExtended = 1.0;
    constexpr double kStairSwitchRetracted = 0.0;
    constexpr double kClickspotPressed = 1.0;
    constexpr int kStairSettledHoldTicks = 2;
    constexpr auto kStairPositionLVar = "EXT_Door_stairs_pos";
    constexpr double kStairStowedPosition = 55.0;

    constexpr auto kModuleFuelMirrorLVar = "146_FuelWeight_KG";
    constexpr double kFuelMovingKgPerTick = 2.0;
    constexpr double kMirrorDivergenceKg = 50.0;
    constexpr int kModuleDeadTicks = 5;

    constexpr int kEngineCount = 4;

    double GroupCapacityGallons(VariableGateway& variables, const std::span<const FuelTank> tanks)
    {
        double capacity = 0.0;
        for (const auto& tank : tanks)
        {
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
      airstairRule_(*this),
      rules_{&airstairRule_}
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

void AvroRj::OnTick()
{
    Observe();
    UpdateAirstairTravel();
    UpdateDoors();
    UpdateAftDoorClosed();
    UpdateAirstair();
    UpdateModuleLiveness();
}

void AvroRj::HoldDoorsClosed(const bool hold)
{
    heldForDeparture_ = hold;
    doors_.HoldClosedForDeparture(hold);
}

void AvroRj::UpdateDoors()
{
    doors_.Report();

    if (IsFrontDoorWanted())
    {
        if (lastFrontDoorTarget_ != kDoorOpen)
        {
            lastFrontDoorTarget_ = kDoorOpen;
            probe::Line(QStringLiteral("write front FwdPax open=1"));
            variableGateway_->SetLVar(kFwdPaxDoorLVar, kDoorOpen);
        }

        return;
    }

    if (lastFrontDoorTarget_ == kDoorOpen && airstairPhase_ == AirstairPhase::Stowed
        && variableGateway_->HasReceivedLVar(kStairPositionLVar)
        && variableGateway_->GetLVar(kStairPositionLVar, 0.0) <= kStairStowedPosition)
    {
        lastFrontDoorTarget_ = kDoorClosed;
        probe::Line(QStringLiteral("write front FwdPax open=0"));
        variableGateway_->SetLVar(kFwdPaxDoorLVar, kDoorClosed);
    }
}

void AvroRj::UpdateAftDoorClosed()
{
    if (variableGateway_->GetLVar(kAftPaxDoorLVar, 0.0) != kDoorOpen)
    {
        aftDoorCloseWritten_ = false;

        return;
    }

    if (aftDoorCloseWritten_)
    {
        return;
    }

    aftDoorCloseWritten_ = true;
    probe::Line(QStringLiteral("write aft AftPax open=0"));
    LOG_INFO("Closing the 2L: this aircraft boards through its own airstair at the 1L");
    variableGateway_->SetLVar(kAftPaxDoorLVar, kDoorClosed);
}

void AvroRj::UpdateModuleLiveness()
{
    const double simFuelKg = CurrentFuelKg(*variableGateway_);
    const bool firstSample = lastLivenessSimFuelKg_ < 0.0;
    const bool fuelMoving = !firstSample
        && std::abs(simFuelKg - lastLivenessSimFuelKg_) > kFuelMovingKgPerTick;
    lastLivenessSimFuelKg_ = simFuelKg;

    const double mirrorKg = variableGateway_->GetLVar(kModuleFuelMirrorLVar, 0.0);
    const bool diverged = std::abs(mirrorKg - simFuelKg) > kMirrorDivergenceKg;

    mirrorDivergentTicks_ = fuelMoving && diverged ? mirrorDivergentTicks_ + 1 : 0;

    if (!diverged)
    {
        moduleDeadLogged_ = false;

        return;
    }

    if (!IsModuleMirroringFuel() && !moduleDeadLogged_)
    {
        moduleDeadLogged_ = true;
        LOG_INFO("The aircraft module stopped mirroring the simulator's fuel; its variables may be frozen");
    }
}

bool AvroRj::IsModuleMirroringFuel() const
{
    return mirrorDivergentTicks_ < kModuleDeadTicks;
}

void AvroRj::WantAirstairs(const bool wanted)
{
    ownAirstairsRequested_ = wanted;
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

void AvroRj::UpdateAirstairTravel()
{
    if (!variableGateway_->HasReceivedLVar(kStairPositionLVar))
    {
        stairPositionStillTicks_ = 0;

        return;
    }

    const double position = variableGateway_->GetLVar(kStairPositionLVar, 0.0);
    stairPositionStillTicks_ = position == lastStairPosition_ ? stairPositionStillTicks_ + 1 : 0;
    lastStairPosition_ = position;
}

bool AvroRj::IsAirstairOutOfItsWell() const
{
    return variableGateway_->HasReceivedLVar(kStairPositionLVar)
        && variableGateway_->GetLVar(kStairPositionLVar, 0.0) > kStairStowedPosition;
}

bool AvroRj::IsAirstairMoving() const
{
    return variableGateway_->HasReceivedLVar(kStairPositionLVar)
        && stairPositionStillTicks_ < kStairSettledHoldTicks;
}

bool AvroRj::AreAirstairsSettled() const
{
    return airstairPhase_ == AirstairPhase::Extended
        && IsAirstairOutOfItsWell()
        && stairPositionStillTicks_ >= kStairSettledHoldTicks;
}

void AvroRj::UpdateAirstair()
{
    if (IsAirstairMoving())
    {
        return;
    }

    const bool wanted = IsAirstairWanted();

    switch (airstairPhase_)
    {
    case AirstairPhase::Stowed:
        if (variableGateway_->GetLVar(kStairExtendSwitchLVar, 0.0) == kStairSwitchExtended
            && IsAirstairOutOfItsWell())
        {
            airstairPhase_ = AirstairPhase::Extended;
            break;
        }

        if (!wanted || variableGateway_->GetLVar(kFwdPaxDoorLVar, 0.0) != kDoorOpen)
        {
            break;
        }

        if (!StairPressureReady())
        {
            break;
        }

        probe::Line(QStringLiteral("write airstair arm clickspot=1"));
        variableGateway_->SetLVar(kStairArmClickspotLVar, kClickspotPressed);
        airstairPhase_ = AirstairPhase::Arming;
        break;
    case AirstairPhase::Arming:
        probe::Line(QStringLiteral("write airstair extend switch=1"));
        variableGateway_->SetLVar(kStairExtendSwitchLVar, kStairSwitchExtended);
        airstairPhase_ = AirstairPhase::Extended;
        break;
    case AirstairPhase::Extended:
        if (variableGateway_->GetLVar(kStairExtendSwitchLVar, 0.0) == kStairSwitchRetracted)
        {
            airstairPhase_ = AirstairPhase::Stowed;
            break;
        }

        if (!wanted)
        {
            if (!StairPressureReady())
            {
                break;
            }

            probe::Line(QStringLiteral("write airstair retract switch=0"));
            variableGateway_->SetLVar(kStairExtendSwitchLVar, kStairSwitchRetracted);
            airstairPhase_ = AirstairPhase::Unarming;
        }

        break;
    case AirstairPhase::Unarming:
        probe::Line(QStringLiteral("write airstair stow clickspot=1"));
        variableGateway_->SetLVar(kStairArmClickspotLVar, kClickspotPressed);
        airstairPhase_ = AirstairPhase::Stowed;
        break;
    }
}

bool AvroRj::IsAirstairWanted() const
{
    if (heldForDeparture_ || !ownAirstairsRequested_)
    {
        return false;
    }

    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return false;
    }

    if (IsJetwayAvailable())
    {
        return false;
    }

    return doors_.VehicleState(gsx::lvars::kPassengerStairsFrontState, 0.0)
        < gsx::states::kVehicleDispatched;
}

bool AvroRj::HasStairPressure() const
{
    return variableGateway_->GetLVar(kStairAccumPressureLVar, 0.0) >= kStairMinPressure;
}

bool AvroRj::StairPressureReady()
{
    if (!HasStairPressure())
    {
        if (!stairPressureWaitLogged_)
        {
            stairPressureWaitLogged_ = true;
            LOG_INFO("Airstair is waiting: no accumulator pressure; the pilot recharges it with the AC pump");
        }

        return false;
    }

    stairPressureWaitLogged_ = false;

    return true;
}

bool AvroRj::IsFrontDoorWanted() const
{
    if (heldForDeparture_)
    {
        return false;
    }

    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return false;
    }

    if (doors_.VehicleState(gsx::lvars::kJetway, kJetwayUnavailable) == kJetwayDocked)
    {
        return true;
    }

    return ownAirstairsRequested_
        || gsx::states::AreStairsArriving(doors_.VehicleState(gsx::lvars::kPassengerStairsFrontState, 0.0));
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
