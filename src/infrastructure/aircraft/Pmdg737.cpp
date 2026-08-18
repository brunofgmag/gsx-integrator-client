#include "Pmdg737.h"

#include "../simvars/SimVars.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include "AircraftRegistry.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../pmdg/Pmdg737DataClient.h"
#include "../pmdg/PmdgRouteFile.h"
#include "../pmdg/PmdgTabletClient.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/FlightPlan.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kSimOnGround = "SIM ON GROUND";

    constexpr auto kChocksLVar = "NGXWheelChocks";
    constexpr auto kSmartSwitchLVar = "switch_752_73X";

    constexpr double kLbsPerKg = 2.20462262185;
    constexpr double kPassengerWeightKg = 84.0;

    constexpr int kGroundConnRetryTicks = 5;
    constexpr int kGroundConnMaxAttempts = 10;
    constexpr int kDoorRetryTicks = 5;
    constexpr int kStateQueryTicks = 3;
    constexpr int kDoorMaxAttempts = 2;
    constexpr int kZfwSettleTicks = 5;
    constexpr int kZfwTrimMaxAttempts = 5;
    constexpr double kZfwTrimToleranceKg = 50.0;

    constexpr auto kTitlePax800 = "737-800 PAX";
    constexpr auto kTitleBcf800 = "737-800BCF";
    constexpr auto kTitleBdsf800 = "737-800BDSF";
    constexpr auto kTitleBbj2Short = "737-800 BB2";
    constexpr auto kTitleBbj2 = "737-800 BBJ2";
}

Pmdg737::Pmdg737(VariableGateway* variableGateway,
                 const AutomationStatus* status,
                 const Pmdg737Variant variant,
                 std::unique_ptr<Pmdg737DataGateway> data,
                 std::unique_ptr<PmdgTabletGateway> tablet)
    : variableGateway_(variableGateway),
      status_(status),
      variant_(variant),
      data_(std::move(data)),
      tablet_(std::move(tablet)),
      doors_(variableGateway),
      smartSwitch_(*variableGateway, {kSmartSwitchLVar},
                   [](double, const double max) { return max > 0.0; })
{
    desiredDoor_.fill(-1);
    commandedDoor_.fill(0);
    ticksSinceDoorCommand_.fill(0);
    doorAttempts_.fill(0);
    LOG_INFO("Profile loaded: %s", GetName());
}

const char* Pmdg737::GetName() const
{
    switch (variant_)
    {
    case Pmdg737Variant::Bcf800:
        return kNameBcf800;
    case Pmdg737Variant::Bdsf800:
        return kNameBdsf800;
    case Pmdg737Variant::Bbj2:
        return kNameBbj2;
    default:
        return kNamePax800;
    }
}

bool Pmdg737::IsCargoVariant() const
{
    return variant_ == Pmdg737Variant::Bcf800 || variant_ == Pmdg737Variant::Bdsf800;
}

std::optional<Pmdg737Door> Pmdg737::DoorFor(const GsxDoor door)
{
    switch (door)
    {
    case GsxDoor::FwdPax: return Pmdg737Door::FwdEntry;
    case GsxDoor::FwdCatering: return Pmdg737Door::FwdService;
    case GsxDoor::AftPax: return Pmdg737Door::AftEntry;
    case GsxDoor::AftCatering: return Pmdg737Door::AftService;
    case GsxDoor::FwdCargo: return Pmdg737Door::FwdCargo;
    case GsxDoor::AftCargo: return Pmdg737Door::AftCargo;
    default: return std::nullopt;
    }
}

void Pmdg737::OnTick()
{
    data_->SetInFlight(variableGateway_->GetAVar(kSimOnGround, kBoolUnit, 1.0) <= 0.0);
    data_->Poll();
    tablet_->Poll();

    if (++ticksSinceStateQuery_ >= kStateQueryTicks)
    {
        ticksSinceStateQuery_ = 0;
        tablet_->RequestState();
    }

    if (!routeFileSeen_ && status_->flightPlanStatus == FlightPlanStatus::Ready)
    {
        routeFileSeen_ = RouteFileMatchesPlan();
    }

    if (data_->HasData())
    {
        smartSwitch_.Subscribe();
        SyncDoors();
        ReconcileGroundConn();
        TrimZfw();
    }
}

bool Pmdg737::RouteFileMatchesPlan() const
{
    const std::optional<std::filesystem::path> directory = PmdgRouteFile::DirectoryFor(GetName());
    if (!directory.has_value())
    {
        return false;
    }

    return PmdgRouteFile::ImportedSince(*directory, status_->plannedOrigin, status_->plannedDestination,
                                        status_->planGeneratedEpoch);
}

void Pmdg737::SyncDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kAutomationDoors, 1.0) != 0.0)
    {
        variableGateway_->SetLVar(gsx::lvars::kAutomationDoors, 0.0);
    }

    doors_.Sync([this](const GsxDoor door, const bool open) { SetDesiredDoor(door, open); });

    if (IsCargoVariant())
    {
        const bool mainLoaderPresent = gsx::states::IsLoaderAtDoor(
            variableGateway_->GetLVar(gsx::lvars::kBaggageLoaderMainState, 0.0));
        desiredDoor_[static_cast<std::size_t>(Pmdg737Door::MainCargo)] = mainLoaderPresent ? 1 : 0;
    }

    ReconcileDoors();
}

void Pmdg737::SetDesiredDoor(const GsxDoor door, const bool open)
{
    const std::optional<Pmdg737Door> target = DoorFor(door);
    if (!target.has_value())
    {
        return;
    }

    desiredDoor_[static_cast<std::size_t>(*target)] = open ? 1 : 0;
}

const char* Pmdg737::EfbDoorKey(const Pmdg737Door door)
{
    switch (door)
    {
    case Pmdg737Door::FwdEntry: return "entry1_left";
    case Pmdg737Door::FwdService: return "entry1_right";
    case Pmdg737Door::AftEntry: return "entry2_left";
    case Pmdg737Door::AftService: return "entry2_right";
    case Pmdg737Door::FwdCargo: return "fwd_cargo";
    case Pmdg737Door::AftCargo: return "aft_cargo";
    case Pmdg737Door::MainCargo: return "main_cargo";
    case Pmdg737Door::EquipmentHatch: return "equipment_hatch";
    default: return nullptr;
    }
}

std::optional<bool> Pmdg737::DoorIsOpen(const Pmdg737Door door) const
{
    const char* key = EfbDoorKey(door);

    return key == nullptr ? std::nullopt : tablet_->DoorOpen(key);
}

void Pmdg737::ReconcileDoors()
{
    for (std::size_t i = 0; i < desiredDoor_.size(); ++i)
    {
        if (desiredDoor_[i] < 0)
        {
            continue;
        }

        const auto door = static_cast<Pmdg737Door>(i);
        const char* key = EfbDoorKey(door);
        if (key != nullptr && tablet_->DoorMoving(key))
        {
            continue;
        }

        const bool wantOpen = desiredDoor_[i] == 1;
        const std::optional<bool> reading = DoorIsOpen(door);
        const bool isOpen = reading.value_or(commandedDoor_[i] == 1);

        if (commandedDoor_[i] != desiredDoor_[i])
        {
            commandedDoor_[i] = desiredDoor_[i];
            ticksSinceDoorCommand_[i] = 0;
            doorAttempts_[i] = 0;

            if (isOpen != wantOpen)
            {
                data_->ToggleDoor(door);
            }

            continue;
        }

        if (!reading.has_value() || isOpen == wantOpen)
        {
            doorAttempts_[i] = 0;

            continue;
        }

        ++ticksSinceDoorCommand_[i];
        if (ticksSinceDoorCommand_[i] >= kDoorRetryTicks && doorAttempts_[i] < kDoorMaxAttempts)
        {
            ticksSinceDoorCommand_[i] = 0;
            ++doorAttempts_[i];
            data_->ToggleDoor(door);
        }
    }
}

void Pmdg737::OnLoadingStarted()
{
    lastSentFuelLbs_ = -1;
    lastSentPax_ = -1;
    lastSentCargoLbs_ = -1;
    lastRequestedZfwKg_ = 0.0;
    zfwSettledTicks_ = 0;
    zfwTrims_ = 0;
}

void Pmdg737::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, const bool open) { SetDesiredDoor(door, open); });

    if (IsCargoVariant())
    {
        desiredDoor_[static_cast<std::size_t>(Pmdg737Door::MainCargo)] = 0;
    }

    ReconcileDoors();
}

bool Pmdg737::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready
        && (tablet_->EfbPlanImported() || routeFileSeen_);
}

double Pmdg737::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double Pmdg737::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int Pmdg737::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double Pmdg737::GetEmptyZfwKg() const
{
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

double Pmdg737::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void Pmdg737::SetCurrentFuelKg(const double fuelKg)
{
    if (!tablet_->IsAvailable())
    {
        return;
    }

    const int lbs = static_cast<int>(std::lround(fuelKg * kLbsPerKg));
    if (lbs == lastSentFuelLbs_)
    {
        return;
    }

    lastSentFuelLbs_ = lbs;
    tablet_->SendFuelTotalLbs(lbs);
}

double Pmdg737::GetCurrentZfwKg() const
{
    const double emptyZfwKg = GetEmptyZfwKg();
    const double totalWeightKg = variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, emptyZfwKg);
    const double zfwKg = totalWeightKg - GetCurrentFuelKg();

    return zfwKg < emptyZfwKg ? emptyZfwKg : zfwKg;
}

void Pmdg737::SetCurrentZfwKg(const double zfwKg)
{
    if (!tablet_->IsAvailable() || !variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return;
    }

    const double emptyZfwKg = GetEmptyZfwKg();
    const double payloadSpanKg = GetPlannedZfwKg() - emptyZfwKg;
    if (payloadSpanKg <= 0.0)
    {
        return;
    }

    const double progress = std::clamp((zfwKg - emptyZfwKg) / payloadSpanKg, 0.0, 1.0);

    double plannedCargoKg = payloadSpanKg;
    if (!IsCargoVariant())
    {
        plannedCargoKg = (std::max)(payloadSpanKg - GetPlannedPassengers() * kPassengerWeightKg, 0.0);

        const int pax = static_cast<int>(std::lround(progress * GetPlannedPassengers()));
        if (pax != lastSentPax_)
        {
            lastSentPax_ = pax;
            tablet_->SendPaxTotal(pax);
        }
    }

    const int cargoLbs = static_cast<int>(std::lround(progress * plannedCargoKg * kLbsPerKg));
    if (cargoLbs != lastSentCargoLbs_)
    {
        lastSentCargoLbs_ = cargoLbs;
        tablet_->SendCargoTotalLbs(cargoLbs);
    }

    if (lastRequestedZfwKg_ != zfwKg)
    {
        lastRequestedZfwKg_ = zfwKg;
        zfwSettledTicks_ = 0;
        zfwTrims_ = 0;
    }
}

void Pmdg737::TrimZfw()
{
    if (lastRequestedZfwKg_ <= 0.0 || lastSentCargoLbs_ < 0 || !tablet_->IsAvailable()
        || zfwTrims_ >= kZfwTrimMaxAttempts)
    {
        return;
    }

    if (++zfwSettledTicks_ < kZfwSettleTicks)
    {
        return;
    }

    const double errorKg = GetCurrentZfwKg() - lastRequestedZfwKg_;
    if (std::abs(errorKg) <= kZfwTrimToleranceKg)
    {
        return;
    }

    const int trimmedLbs =
        (std::max)(lastSentCargoLbs_ - static_cast<int>(std::lround(errorKg * kLbsPerKg)), 0);
    if (trimmedLbs == lastSentCargoLbs_)
    {
        zfwTrims_ = kZfwTrimMaxAttempts;
        return;
    }

    zfwSettledTicks_ = 0;
    ++zfwTrims_;
    lastSentCargoLbs_ = trimmedLbs;
    tablet_->SendCargoTotalLbs(trimmedLbs);
}

bool Pmdg737::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool Pmdg737::ChocksSet() const
{
    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

bool Pmdg737::IsPowered() const
{
    const bool isEngineCombusting =
        variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 0.0) > 0.0
        || variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 0.0) > 0.0;

    return data_->AnyMainBusPowered() || isEngineCombusting;
}

std::optional<GroundPowerStatus> Pmdg737::GetGroundPowerStatus() const
{
    if (!data_->HasData())
    {
        return GroundPowerStatus::Unknown;
    }

    return data_->GroundPowerAvailable() ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

bool Pmdg737::SetChocks(const bool placed)
{
    if (desiredChocks_ != placed)
    {
        desiredChocks_ = placed;
        chocksAttempts_ = 0;
        ticksSinceChocksRequest_ = kGroundConnRetryTicks;
    }

    return true;
}

void Pmdg737::SetGroundPower(const bool on)
{
    if (desiredGroundPower_ != on)
    {
        desiredGroundPower_ = on;
        groundPowerAttempts_ = 0;
        ticksSinceGroundPowerRequest_ = kGroundConnRetryTicks;
    }
}

void Pmdg737::ReconcileGroundConn()
{
    if (desiredChocks_.has_value() && ChocksSet() != *desiredChocks_)
    {
        ++ticksSinceChocksRequest_;
        if (ticksSinceChocksRequest_ >= kGroundConnRetryTicks && chocksAttempts_ < kGroundConnMaxAttempts)
        {
            ticksSinceChocksRequest_ = 0;
            ++chocksAttempts_;
            tablet_->RequestGroundConn("wheel_chocks");
        }
    }
    else
    {
        chocksAttempts_ = 0;
    }

    if (!desiredGroundPower_.has_value())
    {
        return;
    }

    if (data_->GroundPowerAvailable() == *desiredGroundPower_)
    {
        groundPowerAttempts_ = 0;
        return;
    }

    ++ticksSinceGroundPowerRequest_;
    if (ticksSinceGroundPowerRequest_ >= kGroundConnRetryTicks
        && groundPowerAttempts_ < kGroundConnMaxAttempts)
    {
        ticksSinceGroundPowerRequest_ = 0;
        ++groundPowerAttempts_;
        tablet_->RequestGroundConn("ground_power");
    }
}

bool Pmdg737::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && data_->BeaconOn();
}

bool Pmdg737::IsReadyToDeboard() const
{
    return !IsEngineRunning() && (IsParkingBrakeSet() || ChocksSet()) && !data_->BeaconOn();
}

bool Pmdg737::IsEngineRunning() const
{
    const bool isEng1Running = variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng2Running = variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 1.0) > 0.0;

    return isEng1Running || isEng2Running;
}

bool Pmdg737::IsParkingBrakeSet() const
{
    return data_->ParkingBrakeOn()
        || variableGateway_->GetAVar(kSimParkingBrake, kBoolUnit, 0.0) > 0.0;
}

namespace
{
    Pmdg737Variant VariantFor(const AircraftIdentity& identity)
    {
        if (MatchText(identity.title, MatchOp::StartsWith, kTitleBcf800))
        {
            return Pmdg737Variant::Bcf800;
        }

        if (MatchText(identity.title, MatchOp::StartsWith, kTitleBdsf800))
        {
            return Pmdg737Variant::Bdsf800;
        }

        if (MatchText(identity.title, MatchOp::StartsWith, kTitleBbj2Short)
            || MatchText(identity.title, MatchOp::StartsWith, kTitleBbj2))
        {
            return Pmdg737Variant::Bbj2;
        }

        return Pmdg737Variant::Pax800;
    }

    std::unique_ptr<Aircraft> CreatePmdg737(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return std::make_unique<Pmdg737>(
            context.variableGateway, context.status, VariantFor(identity),
            std::make_unique<Pmdg737DataClient>(),
            std::make_unique<PmdgTabletClient>(context.commBusBridge));
    }

    const AircraftDescriptor kPmdg737Pax800Descriptor{
        Pmdg737::kNamePax800,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitlePax800}
        },
        &CreatePmdg737, "pmdg-737-800", "73H", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg737Bcf800Descriptor{
        Pmdg737::kNameBcf800,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitleBcf800}
        },
        &CreatePmdg737, "pmdg-737-800bcf", "73BCF", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg737Bdsf800Descriptor{
        Pmdg737::kNameBdsf800,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitleBdsf800}
        },
        &CreatePmdg737, "pmdg-737-800bdsf", "73SF", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg737Bbj2Descriptor{
        Pmdg737::kNameBbj2,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitleBbj2Short},
            {MatchField::Title, MatchOp::StartsWith, kTitleBbj2}
        },
        &CreatePmdg737, "pmdg-737-bbj2", "73BBJ", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kPmdg737Pax800Registration{kPmdg737Pax800Descriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg737Bcf800Registration{kPmdg737Bcf800Descriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg737Bdsf800Registration{kPmdg737Bdsf800Descriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg737Bbj2Registration{kPmdg737Bbj2Descriptor};
}
