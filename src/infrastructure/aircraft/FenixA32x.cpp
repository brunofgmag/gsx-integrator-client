#include "FenixA32x.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "AircraftRegistry.h"
#include "../fenix/FenixEfbClient.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../domain/ports/GsxGateway.h"
#include "../../domain/turnaround/TurnaroundMath.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kKgUnit = "kg";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kSmartSwitchLVar = "S_ASP_INTRAD";
    constexpr double kSmartSwitchNeutral = 1.0;
    constexpr double kSmartSwitchIntercom = 0.0;

    constexpr auto kParkingBrakeLVar = "S_MIP_PARKING_BRAKE";
    constexpr auto kDcEssBusPoweredLVar = "B_ELEC_BUS_POWER_DC_ESS";
    constexpr auto kBattery1LVar = "S_OH_ELEC_BAT1";
    constexpr auto kBattery2LVar = "S_OH_ELEC_BAT2";
    constexpr auto kExtPowerOnBusLVar = "I_OH_ELEC_EXT_PWR_L";
    constexpr auto kApuRunningLVar = "I_OH_ELEC_APU_START_U";
    constexpr auto kGpuPlacedLVar = "B_CONFIG_GPU";
    constexpr auto kChocksLVar = "B_CONFIG_CHOCKS";
    constexpr auto kThirdPartyRefuelLVar = "S_THIRD_PARTY_REFUELG";

    constexpr auto kChocksDataref = "fenix.efb.chocks";
    constexpr auto kGroundPowerDataref = "groundservice.groundpower";

    constexpr auto kWeightUnitDataref = "system.config.Units.Weight";

    constexpr auto kFuelAmountDataref = "aircraft.fuel.total.amount.kg";
    constexpr auto kFuelTargetDataref = "aircraft.refuel.fuelTarget.kg";
    constexpr auto kCargoTargetDataref = "fenix.efb.plannedCargoKg";
    constexpr auto kSimbriefImportedDataref = "fenix.efb.simbriefPlanImported";
    constexpr auto kBookedSeatsDataref = "fenix.efb.passengers.booked";
    constexpr auto kSeatOccupationStringDataref = "aircraft.passengers.seatOccupation.string";
    constexpr auto kFwdCargoAmountDataref = "aircraft.cargo.forward.amount";
    constexpr auto kAftCargoAmountDataref = "aircraft.cargo.aft.amount";
    constexpr auto kBulkCargoAmountDataref = "aircraft.cargo.bulk.amount";

    constexpr auto kFwdPaxDoorDataref = "doors.entry.d1l";
    constexpr auto kMidPaxDoorDataref = "doors.entry.d2l";
    constexpr auto kAftPaxDoorDataref = "doors.entry.d4l";
    constexpr auto kFwdCateringDoorDataref = "doors.entry.d1r";
    constexpr auto kAftCateringDoorDataref = "doors.entry.d4r";
    constexpr auto kFwdCargoDoorDataref = "doors.cargo.forward";
    constexpr auto kAftCargoDoorDataref = "doors.cargo.aft";

    const char* DoorDataref(const GsxDoor door)
    {
        switch (door)
        {
        case GsxDoor::FwdPax:
            return kFwdPaxDoorDataref;
        case GsxDoor::MidPax:
            return kMidPaxDoorDataref;
        case GsxDoor::AftPax:
            return kAftPaxDoorDataref;
        case GsxDoor::FwdCatering:
            return kFwdCateringDoorDataref;
        case GsxDoor::AftCatering:
            return kAftCateringDoorDataref;
        case GsxDoor::FwdCargo:
            return kFwdCargoDoorDataref;
        default:
            return kAftCargoDoorDataref;
        }
    }

    constexpr std::array kAutomationTogglesToDisable = {
        "fenix.efb.autoDoor",
        "fenix.efb.autoJetway",
        "fenix.gsx.autoConnectGpu",
        "fenix.gsx.autoDisconnectGpu",
        "fenix.gsx.autoDeboard",
        "fenix.gsx.autoCatering",
        "fenix.gsx.autoPushback",
        "fenix.gsx.autoSelectOperator"
    };

    constexpr auto kLoadsheetPreliminary = "Preliminary";
    constexpr auto kLoadsheetFinal = "Final";

    constexpr double kPassengerWeightKg = 84.0;
    constexpr double kCargoShareForward = 0.4237;
    constexpr double kCargoShareAft = 0.4237;

    std::string BuildSeatString(const std::vector<bool>& bookedSeats, const int occupiedCount)
    {
        std::string seats;
        int remaining = occupiedCount;
        for (std::size_t i = 0; i < bookedSeats.size(); ++i)
        {
            const bool occupied = bookedSeats[i] && remaining > 0;
            if (occupied)
            {
                --remaining;
            }

            if (i > 0)
            {
                seats += ',';
            }
            seats += occupied ? "true" : "false";
        }

        return seats;
    }
}

FenixA32x::FenixA32x(VariableGateway* variableGateway, const FenixVariant variant, std::unique_ptr<FenixEfbGateway> efb)
    : variableGateway_(variableGateway),
      variant_(variant),
      efb_(std::move(efb)),
      doors_(variableGateway),
      smartSwitch_(*variableGateway, {kSmartSwitchLVar},
                   [](const double min, double) { return min <= kSmartSwitchIntercom; },
                   kSmartSwitchNeutral)
{
    smartSwitch_.Subscribe();
    efb_->Subscribe(kSimbriefImportedDataref);
    efb_->Subscribe(kBookedSeatsDataref);
    efb_->Subscribe(kFuelTargetDataref);
    efb_->Subscribe(kCargoTargetDataref);
    efb_->Subscribe(kWeightUnitDataref);

    LOG_INFO("Profile loaded: %s", GetName());
}

const char* FenixA32x::GetName() const
{
    switch (variant_)
    {
    case FenixVariant::A319:
        return kNameA319;
    case FenixVariant::A321:
        return kNameA321;
    default:
        return kNameA320;
    }
}

bool FenixA32x::IsCargoVariant() const
{
    return false;
}

void FenixA32x::OnTick()
{
    efb_->Poll();
    EnsureEfbInitialized();
    UpdateDoors();
    DisarmRefuelSystemWhenDone();
}

void FenixA32x::EnsureEfbInitialized()
{
    if (!efb_->IsAvailable())
    {
        efbInitialized_ = false;

        return;
    }

    if (efbInitialized_)
    {
        return;
    }

    efbInitialized_ = true;
    for (const auto* toggle : kAutomationTogglesToDisable)
    {
        efb_->SetBool(toggle, false);
    }

    LOG_INFO("Fenix EFB automation disabled: the client now controls doors and GSX services");
}

void FenixA32x::OnLoadingStarted()
{
    finalLoadsheetRequested_ = false;
    refuelSystemArmed_ = true;
    variableGateway_->SetLVar(kThirdPartyRefuelLVar, 1.0);

    LOG_INFO("Fenix third-party refueling armed: GSX can connect the fuel hose");

    if (efb_->IsAvailable())
    {
        efb_->RequestLoadsheet(kLoadsheetPreliminary);
    }
}

void FenixA32x::DisarmRefuelSystemWhenDone()
{
    if (!refuelSystemArmed_ || variableGateway_->GetLVar(gsx::lvars::kRefuelingState, 0.0)
        != static_cast<double>(GsxStateStatus::Completed))
    {
        return;
    }

    refuelSystemArmed_ = false;
    variableGateway_->SetLVar(kThirdPartyRefuelLVar, 0.0);
}

void FenixA32x::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, bool)
    {
        efb_->SetBool(DoorDataref(door), false);
    });

    LOG_INFO("All doors commanded closed: door control is now manual");
}

void FenixA32x::UpdateDoors()
{
    if (!efb_->IsAvailable())
    {
        return;
    }

    doors_.Sync([this](const GsxDoor door, const bool open)
    {
        if (door == GsxDoor::MidPax && variant_ != FenixVariant::A321)
        {
            return;
        }

        efb_->SetBool(DoorDataref(door), open);
    });
}

bool FenixA32x::IsFlightPlanLoaded() const
{
    return variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit)
        && efb_->GetNumber(kSimbriefImportedDataref, 0.0) > 0.0
        && efb_->GetNumber(kFuelTargetDataref, 0.0) > 0.0;
}

double FenixA32x::GetPlannedFuelKg() const
{
    return efb_->GetNumber(kFuelTargetDataref, 0.0);
}

double FenixA32x::GetPlannedZfwKg() const
{
    return GetEmptyZfwKg() + GetPlannedPassengers() * kPassengerWeightKg + PlannedCargoKg();
}

int FenixA32x::GetPlannedPassengers() const
{
    const std::vector<bool> bookedSeats = efb_->GetBoolArray(kBookedSeatsDataref);

    return static_cast<int>(std::ranges::count(bookedSeats, true));
}

double FenixA32x::GetEmptyZfwKg() const
{
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

std::optional<WeightUnit> FenixA32x::GetNativeWeightUnit() const
{
    std::string unit = efb_->GetString(kWeightUnitDataref, "");
    std::ranges::transform(unit, unit.begin(),
                           [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (unit == "KG")
    {
        return WeightUnit::Kg;
    }
    if (unit == "LBS" || unit == "LB")
    {
        return WeightUnit::Lb;
    }

    return std::nullopt;
}

double FenixA32x::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void FenixA32x::SetCurrentFuelKg(const double fuelKg)
{
    if (!efb_->IsAvailable() || fuelKg == lastFuelKg_)
    {
        return;
    }

    lastFuelKg_ = fuelKg;
    efb_->SetFloat(kFuelAmountDataref, fuelKg);
}

double FenixA32x::GetCurrentZfwKg() const
{
    const double totalWeightKg =
        variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, GetEmptyZfwKg());

    return std::max(totalWeightKg - GetCurrentFuelKg(), GetEmptyZfwKg());
}

void FenixA32x::SetCurrentZfwKg(const double zfwKg)
{
    if (!efb_->IsAvailable()
        || !variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit)
        || zfwKg == lastZfwKg_)
    {
        return;
    }

    lastZfwKg_ = zfwKg;
    SyncPassengersAndCargo(zfwKg);
    MaybeRequestFinalLoadsheet(zfwKg);
}

double FenixA32x::PlannedCargoKg() const
{
    return efb_->GetNumber(kCargoTargetDataref, 0.0);
}

void FenixA32x::SyncPassengersAndCargo(const double zfwKg)
{
    const double emptyZfwKg = GetEmptyZfwKg();
    const double payloadSpanKg = GetPlannedZfwKg() - emptyZfwKg;
    if (payloadSpanKg <= 0.0)
    {
        return;
    }

    const double progress = std::clamp((zfwKg - emptyZfwKg) / payloadSpanKg, 0.0, 1.0);

    WriteSeatOccupation(static_cast<int>(std::lround(progress * GetPlannedPassengers())));

    const double cargoKg = progress * PlannedCargoKg();
    const double fwdCargoKg = cargoKg * kCargoShareForward;
    const double aftCargoKg = cargoKg * kCargoShareAft;

    efb_->SetFloat(kFwdCargoAmountDataref, fwdCargoKg);
    efb_->SetFloat(kAftCargoAmountDataref, aftCargoKg);
    efb_->SetFloat(kBulkCargoAmountDataref, cargoKg - fwdCargoKg - aftCargoKg);
}

void FenixA32x::WriteSeatOccupation(const int passengersOnBoard)
{
    if (passengersOnBoard == lastPassengersOnBoard_)
    {
        return;
    }

    const std::vector<bool> bookedSeats = efb_->GetBoolArray(kBookedSeatsDataref);
    if (bookedSeats.empty())
    {
        return;
    }

    lastPassengersOnBoard_ = passengersOnBoard;
    efb_->SetString(kSeatOccupationStringDataref, BuildSeatString(bookedSeats, passengersOnBoard));
}

void FenixA32x::MaybeRequestFinalLoadsheet(const double zfwKg)
{
    if (finalLoadsheetRequested_)
    {
        return;
    }

    const double plannedZfwKg = GetPlannedZfwKg();
    if (plannedZfwKg <= GetEmptyZfwKg() || zfwKg < plannedZfwKg - turnaround::kWeightEpsilonKg)
    {
        return;
    }

    finalLoadsheetRequested_ = true;
    efb_->RequestLoadsheet(kLoadsheetFinal);
}

bool FenixA32x::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool FenixA32x::IsPowered() const
{
    const bool isBusPowered = variableGateway_->GetLVar(kDcEssBusPoweredLVar, 0.0) > 0.0;
    const bool isBatteryOn = variableGateway_->GetLVar(kBattery1LVar, 0.0) > 0.0
        || variableGateway_->GetLVar(kBattery2LVar, 0.0) > 0.0;
    const bool isExtPowerOn = variableGateway_->GetLVar(kExtPowerOnBusLVar, 0.0) > 0.0;
    const bool isApuRunning = variableGateway_->GetLVar(kApuRunningLVar, 0.0) > 0.0;

    if (!isBusPowered && !isBatteryOn)
    {
        return false;
    }

    if (isBatteryOn && !isExtPowerOn && !isApuRunning)
    {
        return false;
    }

    return true;
}

std::optional<GroundPowerStatus> FenixA32x::GetGroundPowerStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kGpuPlacedLVar))
    {
        return GroundPowerStatus::Unknown;
    }

    const bool connected = variableGateway_->GetLVar(kGpuPlacedLVar, 0.0) > 0.0
        || variableGateway_->GetLVar(kExtPowerOnBusLVar, 0.0) > 0.0;

    return connected ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

void FenixA32x::SetChocks(const bool placed)
{
    efb_->SetBool(kChocksDataref, placed);
}

void FenixA32x::SetGroundPower(const bool on)
{
    efb_->SetBool(kGroundPowerDataref, on);
}

bool FenixA32x::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool FenixA32x::IsReadyToDeboard() const
{
    const bool isChocksOn = variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;

    return !IsEngineRunning() && (IsParkingBrakeSet() || isChocksOn) && !IsBeaconOn();
}

bool FenixA32x::IsEngineRunning() const
{
    const bool isEng1Running = variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng2Running = variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 1.0) > 0.0;

    return isEng1Running || isEng2Running;
}

bool FenixA32x::IsParkingBrakeSet() const
{
    const bool isParkingBrakeOn = variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
    const bool isSimParkingBrakeOn = variableGateway_->GetAVar(kSimParkingBrake, kBoolUnit, 0.0) > 0.0;

    return isParkingBrakeOn && isSimParkingBrakeOn;
}

bool FenixA32x::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    FenixVariant VariantFor(const AircraftIdentity& identity)
    {
        if (MatchText(identity.title, MatchOp::StartsWith, "FenixA319"))
        {
            return FenixVariant::A319;
        }

        if (MatchText(identity.title, MatchOp::StartsWith, "FenixA321"))
        {
            return FenixVariant::A321;
        }

        return FenixVariant::A320;
    }

    std::unique_ptr<Aircraft> CreateFenixA32x(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return std::make_unique<FenixA32x>(context.variableGateway,
                                           VariantFor(identity),
                                           std::make_unique<FenixEfbClient>());
    }

    const AircraftDescriptor kFenixA319Descriptor{
        FenixA32x::kNameA319,
        {
            {MatchField::Title, MatchOp::StartsWith, "FenixA319"}
        },
        &CreateFenixA32x, "fenix-a319", "A319", RefuelBy::Client
    };

    const AircraftDescriptor kFenixA320Descriptor{
        FenixA32x::kNameA320,
        {
            {MatchField::Title, MatchOp::StartsWith, "FenixA320"}
        },
        &CreateFenixA32x, "fenix-a320", "A320", RefuelBy::Client
    };

    const AircraftDescriptor kFenixA321Descriptor{
        FenixA32x::kNameA321,
        {
            {MatchField::Title, MatchOp::StartsWith, "FenixA321"}
        },
        &CreateFenixA32x, "fenix-a321", "A321", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kFenixA319Registration{kFenixA319Descriptor};
    [[maybe_unused]] const AircraftRegistration kFenixA320Registration{kFenixA320Descriptor};
    [[maybe_unused]] const AircraftRegistration kFenixA321Registration{kFenixA321Descriptor};
}
