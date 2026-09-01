#include "FenixA32x.h"

#include "../DoorReading.h"
#include "../../probe/ProbeWatchList.h"
#include "../../simvars/SimVars.h"

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
#include <QtCore/QStringList>
#include "../AircraftRegistry.h"
#include "../../fenix/FenixEfbClient.h"
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../../domain/ports/GsxGateway.h"
#include "../../../domain/turnaround/TurnaroundMath.h"
#include "../../../infrastructure/simvars/VariableGateway.h"
#include "../../probe/ProbeLog.h"

using namespace simvars;

namespace
{
    constexpr auto kSmartSwitchLVar = "S_ASP_INTRAD";
    constexpr double kSmartSwitchNeutral = 1.0;
    constexpr double kSmartSwitchIntercom = 0.0;

    constexpr int kEngineCount = 2;
    constexpr double kEngineRunningDefault = 1.0;

    constexpr auto kParkingBrakeLVar = "S_MIP_PARKING_BRAKE";
    constexpr auto kDcEssBusPoweredLVar = "B_ELEC_BUS_POWER_DC_ESS";
    constexpr auto kBattery1LVar = "S_OH_ELEC_BAT1";
    constexpr auto kBattery2LVar = "S_OH_ELEC_BAT2";
    constexpr auto kExtPowerOnBusLVar = "I_OH_ELEC_EXT_PWR_L";
    constexpr auto kApuRunningLVar = "I_OH_ELEC_APU_START_U";
    constexpr auto kGpuPlacedLVar = "B_CONFIG_GPU";
    constexpr auto kChocksLVar = "B_CONFIG_CHOCKS";

    constexpr auto kChocksDataref = "fenix.efb.chocks";
    constexpr auto kGroundPowerDataref = "groundservice.groundpower";

    constexpr auto kWeightUnitDataref = "system.config.Units.Weight";

    constexpr auto kFwdPaxDoorDataref = "doors.entry.d1l";
    constexpr auto kMidPaxDoorDataref = "doors.entry.d2l";
    constexpr auto kAftPaxDoorDataref = "doors.entry.d4l";
    constexpr auto kFwdCateringDoorDataref = "doors.entry.d1r";
    constexpr auto kAftCateringDoorDataref = "doors.entry.d4r";
    constexpr auto kFwdCargoDoorDataref = "doors.cargo.forward";
    constexpr auto kAftCargoDoorDataref = "doors.cargo.aft";

    constexpr auto kThirdLeftDoorCandidate = "doors.entry.d3l";
    constexpr auto kSecondRightDoorCandidate = "doors.entry.d2r";
    constexpr auto kThirdRightDoorCandidate = "doors.entry.d3r";
    constexpr auto kBulkCargoDoorCandidate = "doors.cargo.bulk";

    constexpr std::array kSharedDoorDatarefs = {
        kFwdPaxDoorDataref, kAftPaxDoorDataref,
        kFwdCateringDoorDataref, kAftCateringDoorDataref,
        kFwdCargoDoorDataref, kAftCargoDoorDataref,
        kThirdLeftDoorCandidate, kSecondRightDoorCandidate,
        kThirdRightDoorCandidate, kBulkCargoDoorCandidate
    };

    constexpr std::array kProbeDoorDatarefs = {
        kFwdPaxDoorDataref, kMidPaxDoorDataref, kAftPaxDoorDataref,
        kFwdCateringDoorDataref, kAftCateringDoorDataref,
        kFwdCargoDoorDataref, kAftCargoDoorDataref,
        kThirdLeftDoorCandidate, kSecondRightDoorCandidate,
        kThirdRightDoorCandidate, kBulkCargoDoorCandidate
    };

    constexpr double kDoorUnanswered = -1.0;
    constexpr int kDoorSettleTicks = 8;

    constexpr auto kFuelAmountDataref = "aircraft.fuel.total.amount.kg";
    constexpr auto kFuelTargetDataref = "aircraft.refuel.fuelTarget.kg";
    constexpr auto kCargoTargetDataref = "fenix.efb.plannedCargoKg";
    constexpr auto kSimbriefImportedDataref = "fenix.efb.simbriefPlanImported";
    constexpr auto kBookedSeatsDataref = "fenix.efb.passengers.booked";
    constexpr auto kSeatOccupationStringDataref = "aircraft.passengers.seatOccupation.string";
    constexpr auto kFwdCargoAmountDataref = "aircraft.cargo.forward.amount";
    constexpr auto kAftCargoAmountDataref = "aircraft.cargo.aft.amount";
    constexpr auto kBulkCargoAmountDataref = "aircraft.cargo.bulk.amount";

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
                   kSmartSwitchNeutral),
      efbSetupRule_(*efb_),
      doorRule_(*this, *efb_, doors_, variant),
      refuelSystemRule_(*variableGateway),
      rules_{&efbSetupRule_, &doorRule_, &refuelSystemRule_}
{
    smartSwitch_.Subscribe();
    efb_->Subscribe(kSimbriefImportedDataref);
    efb_->Subscribe(kBookedSeatsDataref);
    efb_->Subscribe(kFuelTargetDataref);
    efb_->Subscribe(kCargoTargetDataref);
    efb_->Subscribe(kWeightUnitDataref);

    for (const char* const dataref : kSharedDoorDatarefs)
    {
        efb_->Subscribe(dataref);
    }

    if (variant_ == FenixVariant::A321)
    {
        efb_->Subscribe(kMidPaxDoorDataref);
    }

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

void FenixA32x::Observe()
{
    doors_.Observe();
    efb_->Poll();
    AdvanceDoorSettle();
    ReportProbe();
}

const std::vector<AircraftRule*>& FenixA32x::Rules() const
{
    return rules_;
}

void FenixA32x::ReportProbe() const
{
    if (!probe::IsOn())
    {
        return;
    }

    QStringList doors;
    for (const char* const dataref : kProbeDoorDatarefs)
    {
        doors.append(QStringLiteral("%1=%2").arg(QLatin1String(dataref))
                     .arg(efb_->GetNumber(dataref, kDoorUnanswered), 0, 'f', 3));
    }

    probe::Change("fenix.doors", QStringLiteral("efb   fenix available=%1 %2")
                  .arg(efb_->IsAvailable() ? 1 : 0)
                  .arg(doors.join(QLatin1Char(' '))));

    for (const probe::WatchedVariable& watched : probe::WatchList())
    {
        if (watched.kind != probe::WatchKind::Dataref)
        {
            continue;
        }

        probe::Change("watch." + watched.name,
                      QStringLiteral("watch fenix %1=%2")
                      .arg(QString::fromStdString(watched.name))
                      .arg(efb_->GetNumber(watched.name, -1.0), 0, 'f', 3));
    }
}

void FenixA32x::OnLoadingStarted()
{
    finalLoadsheetRequested_ = false;

    if (efb_->IsAvailable())
    {
        efb_->RequestLoadsheet(kLoadsheetPreliminary);
    }
}

const char* FenixA32x::DoorDataref(const GsxDoor door) const
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

void FenixA32x::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, bool)
    {
        efb_->SetBool(DoorDataref(door), false);
    });

    LOG_INFO("All doors commanded closed: door control is now manual");
}

DoorStatus FenixA32x::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* const dataref : kSharedDoorDatarefs)
    {
        status = doors::Combine(status, DoorOpen(dataref));
    }

    if (variant_ == FenixVariant::A321)
    {
        status = doors::Combine(status, DoorOpen(kMidPaxDoorDataref));
    }

    return status;
}

std::optional<bool> FenixA32x::DoorOpen(const char* dataref) const
{
    const double reading = efb_->GetNumber(dataref, kDoorUnanswered);
    if (reading < 0.0)
    {
        return std::nullopt;
    }

    if (reading > 0.0)
    {
        return true;
    }

    const auto settling = doorSettleTicks_.find(dataref);

    return settling != doorSettleTicks_.end() && settling->second > 0
               ? std::optional{true}
               : std::optional{false};
}

void FenixA32x::AdvanceDoorSettle()
{
    for (const char* const dataref : kProbeDoorDatarefs)
    {
        const double reading = efb_->GetNumber(dataref, kDoorUnanswered);
        int& settling = doorSettleTicks_[dataref];
        double& last = lastDoorReading_[dataref];

        if (reading != 0.0)
        {
            settling = 0;
        }
        else if (last > 0.0)
        {
            settling = kDoorSettleTicks;
        }
        else if (settling > 0)
        {
            --settling;
        }

        last = reading;
    }
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
    return EmptyZfwKg(*variableGateway_);
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
    return CurrentFuelKg(*variableGateway_);
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
    return CurrentZfwKg(*variableGateway_);
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
    const double progress = payloadSpanKg > 0.0
                                ? std::clamp((zfwKg - emptyZfwKg) / payloadSpanKg, 0.0, 1.0)
                                : 0.0;

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

bool FenixA32x::SetChocks(const bool placed)
{
    efb_->SetBool(kChocksDataref, placed);

    return true;
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
    return !IsEngineRunning() && IsHeldInPlace() && !IsBeaconOn();
}

bool FenixA32x::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

bool FenixA32x::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool FenixA32x::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
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

void FenixA32x::HoldDoorsClosed(const bool hold)
{
    doors_.HoldClosedForDeparture(hold);
}
