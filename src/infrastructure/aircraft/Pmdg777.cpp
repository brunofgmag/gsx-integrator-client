#include "Pmdg777.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>
#include <QtCore/QString>
#include "AircraftRegistry.h"
#include "../commbus/CommBusBridgeClient.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../pmdg/Pmdg777DataClient.h"
#include "../pmdg/Pmdg777TabletClient.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/FlightPlan.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimOnGround = "SIM ON GROUND";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kKgUnit = "kg";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kSmartSwitchCaptLVar = "switch_554_a";
    constexpr auto kSmartSwitchFoLVar = "switch_773_a";
    constexpr double kSmartSwitchPressed = 100.0;
    constexpr double kLbsPerKg = 2.20462262185;
    constexpr double kPassengerWeightKg = 84.0;

    constexpr int kMainDeckCargoDoor = 12;
    constexpr int kGroundConnRetryTicks = 5;
    constexpr int kGroundConnMaxAttempts = 10;
    constexpr int kZfwSettleTicks = 5;
    constexpr int kZfwTrimMaxAttempts = 5;
    constexpr double kZfwTrimToleranceKg = 50.0;
    constexpr int kDoorStateOpen = 0;
    constexpr int kDoorStateClosing = 3;
    constexpr int kDoorStateOpening = 4;

    constexpr auto kTitle300Er = "777-300ER";
    constexpr auto kTitleFreighter = "777F";
    constexpr auto kTitle200Lr = "777-200LR";
    constexpr auto kTitle200Er = "777-200ER";
}

Pmdg777::Pmdg777(VariableGateway* variableGateway,
                 AutomationStatus* status,
                 const Pmdg777Variant variant,
                 std::unique_ptr<Pmdg777DataGateway> data,
                 std::unique_ptr<Pmdg777TabletGateway> tablet)
    : variableGateway_(variableGateway),
      status_(status),
      variant_(variant),
      data_(std::move(data)),
      tablet_(std::move(tablet)),
      doors_(variableGateway),
      smartSwitch_(*variableGateway, {kSmartSwitchCaptLVar, kSmartSwitchFoLVar},
                   [](double, const double max) { return max >= kSmartSwitchPressed; })
{
    desiredDoor_.fill(-1);
    openedDoorIndex_.fill(-1);
    LOG_INFO("Profile loaded: %s", GetName());
}

bool Pmdg777::OptionsEnableDataBroadcast(const std::string& iniText)
{
    std::istringstream stream(iniText);
    std::string line;
    bool inSdkSection = false;
    while (std::getline(stream, line))
    {
        const auto begin = line.find_first_not_of(" \t\r");
        if (begin == std::string::npos)
        {
            continue;
        }

        if (line[begin] == '[')
        {
            inSdkSection = line.find("[SDK]", begin) == begin;
            continue;
        }

        if (inSdkSection && line.find("EnableDataBroadcast") != std::string::npos
            && line.find('1', line.find('=')) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

const char* Pmdg777::GetName() const
{
    switch (variant_)
    {
    case Pmdg777Variant::Er300:
        return kName300Er;
    case Pmdg777Variant::Freighter:
        return kNameFreighter;
    case Pmdg777Variant::Lr200:
        return kName200Lr;
    default:
        return kName200Er;
    }
}

bool Pmdg777::IsCargoVariant() const
{
    return variant_ == Pmdg777Variant::Freighter;
}

void Pmdg777::OnTick()
{
    data_->SetInFlight(variableGateway_->GetAVar(kSimOnGround, kBoolUnit, 1.0) <= 0.0);
    data_->Poll();
    tablet_->Poll();

    if (data_->HasData())
    {
        smartSwitch_.Subscribe();
    }

    if (data_->HasData())
    {
        SyncDoors();
        ReconcileGroundConn();
        TrimZfw();
    }
}

void Pmdg777::SyncDoors()
{
    if (variableGateway_->GetLVar(gsx::lvars::kAutomationDoors, 1.0) != 0.0)
    {
        variableGateway_->SetLVar(gsx::lvars::kAutomationDoors, 0.0);
    }

    doors_.Sync([this](const GsxDoor door, const bool open) { SetDesiredDoor(door, open); });

    if (IsCargoVariant())
    {
        const bool mainLoaderPresent = gsx::states::IsLoaderPresent(
            variableGateway_->GetLVar(gsx::lvars::kBaggageLoaderMainState, 0.0));
        desiredDoor_[kMainDeckCargoDoor] = mainLoaderPresent ? 1 : 0;
    }

    ReconcileDoors();
}

void Pmdg777::SetDesiredDoor(const GsxDoor door, const bool open)
{
    int& openedIndex = openedDoorIndex_[static_cast<std::size_t>(door)];
    const int index = !open && openedIndex >= 0 ? openedIndex : DoorIndexFor(door);
    if (index < 0)
    {
        return;
    }

    desiredDoor_[static_cast<std::size_t>(index)] = open ? 1 : 0;
    openedIndex = open ? index : -1;
}

void Pmdg777::ReconcileDoors() const
{
    for (std::size_t i = 0; i < desiredDoor_.size(); ++i)
    {
        if (desiredDoor_[i] < 0)
        {
            continue;
        }

        const int state = data_->DoorState(static_cast<int>(i));
        if (state < 0 || state == kDoorStateClosing || state == kDoorStateOpening)
        {
            continue;
        }

        const bool isOpen = state == kDoorStateOpen;
        if (isOpen != (desiredDoor_[i] == 1))
        {
            data_->ToggleDoor(static_cast<int>(i));
        }
    }
}

int Pmdg777::DoorIndexFor(const GsxDoor door) const
{
    if (IsCargoVariant())
    {
        switch (door)
        {
        case GsxDoor::FwdPax: return 0;
        case GsxDoor::FwdCatering: return 1;
        case GsxDoor::FwdCargo: return 10;
        case GsxDoor::AftCargo: return 11;
        default: return -1;
        }
    }

    const bool is300 = variant_ == Pmdg777Variant::Er300;
    switch (door)
    {
    case GsxDoor::FwdPax:
        return variableGateway_->GetLVar(gsx::lvars::kJetway, 2.0) == 5.0 ? 2 : 0;
    case GsxDoor::MidPax: return 2;
    case GsxDoor::AftPax: return is300 ? 4 : 6;
    case GsxDoor::FwdCatering: return is300 ? 3 : 1;
    case GsxDoor::AftCatering: return is300 ? 9 : 7;
    case GsxDoor::FwdCargo: return 10;
    case GsxDoor::AftCargo: return 11;
    default: return -1;
    }
}

void Pmdg777::OnLoadingStarted()
{
    lastSentFuelLbs_ = -1;
    lastSentPax_ = -1;
    lastSentCargoLbs_ = -1;
    lastRequestedZfwKg_ = 0.0;
    zfwSettledTicks_ = 0;
    zfwTrims_ = 0;
}

void Pmdg777::CloseAllDoors()
{
    doors_.CloseAll([this](const GsxDoor door, const bool open) { SetDesiredDoor(door, open); });

    if (IsCargoVariant())
    {
        desiredDoor_[kMainDeckCargoDoor] = 0;
    }

    ReconcileDoors();
}

bool Pmdg777::IsFlightPlanLoaded() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready
        && (tablet_->EfbPlanImported() || data_->HasFmcFlightPlan());
}

double Pmdg777::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double Pmdg777::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int Pmdg777::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double Pmdg777::GetEmptyZfwKg() const
{
    return variableGateway_->GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
}

double Pmdg777::GetCurrentFuelKg() const
{
    return variableGateway_->GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
}

void Pmdg777::SetCurrentFuelKg(const double fuelKg)
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

double Pmdg777::GetCurrentZfwKg() const
{
    const double emptyZfwKg = GetEmptyZfwKg();
    const double totalWeightKg = variableGateway_->GetAVar(kSimTotalWeight, kKgUnit, emptyZfwKg);
    const double zfwKg = totalWeightKg - GetCurrentFuelKg();

    return zfwKg < emptyZfwKg ? emptyZfwKg : zfwKg;
}

void Pmdg777::SetCurrentZfwKg(const double zfwKg)
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

void Pmdg777::TrimZfw()
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

bool Pmdg777::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

bool Pmdg777::IsPowered() const
{
    const bool isEngineCombusting =
        variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 0.0) > 0.0
        || variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 0.0) > 0.0;

    return data_->ApuRunning() || data_->ExtPowerConnected() || isEngineCombusting;
}

std::optional<GroundPowerStatus> Pmdg777::GetGroundPowerStatus() const
{
    if (!data_->HasData())
    {
        return GroundPowerStatus::Unknown;
    }

    return data_->ExtPowerConnected() ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

void Pmdg777::SetChocks(const bool placed)
{
    if (desiredChocks_ != placed)
    {
        desiredChocks_ = placed;
        chocksAttempts_ = 0;
        ticksSinceChocksRequest_ = kGroundConnRetryTicks;
    }
}

void Pmdg777::SetGroundPower(const bool on)
{
    if (desiredGroundPower_ != on)
    {
        desiredGroundPower_ = on;
        groundPowerAttempts_ = 0;
        ticksSinceGroundPowerRequest_ = kGroundConnRetryTicks;
    }
}

void Pmdg777::ReconcileGroundConn()
{
    if (desiredChocks_.has_value() && data_->WheelChocksSet() != *desiredChocks_)
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

    const bool gpuPresent = data_->ExtPowerAvailable() || data_->ExtPowerConnected();
    if (gpuPresent == *desiredGroundPower_)
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

bool Pmdg777::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && data_->BeaconOn();
}

bool Pmdg777::IsReadyToDeboard() const
{
    return !IsEngineRunning() && (IsParkingBrakeSet() || data_->WheelChocksSet()) && !data_->BeaconOn();
}

bool Pmdg777::IsEngineRunning() const
{
    const bool isEng1Running = variableGateway_->GetAVar(kSimEng1Combustion, kBoolUnit, 1.0) > 0.0;
    const bool isEng2Running = variableGateway_->GetAVar(kSimEng2Combustion, kBoolUnit, 1.0) > 0.0;

    return isEng1Running || isEng2Running;
}

bool Pmdg777::IsParkingBrakeSet() const
{
    return data_->ParkingBrakeOn();
}

namespace
{
    Pmdg777Variant VariantFor(const AircraftIdentity& identity)
    {
        if (MatchText(identity.title, MatchOp::StartsWith, kTitle300Er))
        {
            return Pmdg777Variant::Er300;
        }

        if (MatchText(identity.title, MatchOp::StartsWith, kTitleFreighter))
        {
            return Pmdg777Variant::Freighter;
        }

        if (MatchText(identity.title, MatchOp::StartsWith, kTitle200Lr))
        {
            return Pmdg777Variant::Lr200;
        }

        return Pmdg777Variant::Er200;
    }

    const char* PackageFor(const Pmdg777Variant variant)
    {
        switch (variant)
        {
        case Pmdg777Variant::Er300: return "pmdg-aircraft-77w";
        case Pmdg777Variant::Freighter: return "pmdg-aircraft-77f";
        case Pmdg777Variant::Lr200: return "pmdg-aircraft-77l";
        default: return "pmdg-aircraft-77er";
        }
    }

    void WarnWhenDataBroadcastDisabled(const Pmdg777Variant variant)
    {
        const QString appData = qEnvironmentVariable("APPDATA");
        if (appData.isEmpty())
        {
            return;
        }

        const std::filesystem::path ini = std::filesystem::path(appData.toStdWString())
            / "Microsoft Flight Simulator 2024" / "WASM" / "MSFS2024" / PackageFor(variant)
            / "work" / "777_Options.ini";

        std::error_code ec;
        if (!std::filesystem::exists(ini, ec))
        {
            return;
        }

        std::ifstream file(ini);
        const std::string text((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());
        if (!Pmdg777::OptionsEnableDataBroadcast(text))
        {
            LOG_WARN("777_Options.ini has no '[SDK] EnableDataBroadcast=1'; the client cannot read the "
                     "aircraft state. Enable it and restart the flight (%s).", ini.string().c_str());
        }
    }

    std::unique_ptr<Aircraft> CreatePmdg777(VariableGateway* variableGateway,
                                            AutomationStatus* status,
                                            const AircraftIdentity& identity)
    {
        WarnWhenDataBroadcastDisabled(VariantFor(identity));

        return std::make_unique<Pmdg777>(
            variableGateway, status, VariantFor(identity),
            std::make_unique<Pmdg777DataClient>(),
            std::make_unique<Pmdg777TabletClient>(std::make_unique<CommBusBridgeClient>()));
    }

    const AircraftDescriptor kPmdg777300ErDescriptor{
        Pmdg777::kName300Er,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitle300Er}
        },
        &CreatePmdg777, "pmdg-777-300er", "77W", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg777FreighterDescriptor{
        Pmdg777::kNameFreighter,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitleFreighter}
        },
        &CreatePmdg777, "pmdg-777f", "77F", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg777200LrDescriptor{
        Pmdg777::kName200Lr,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitle200Lr}
        },
        &CreatePmdg777, "pmdg-777-200lr", "77L", RefuelBy::Client
    };

    const AircraftDescriptor kPmdg777200ErDescriptor{
        Pmdg777::kName200Er,
        {
            {MatchField::Title, MatchOp::StartsWith, kTitle200Er}
        },
        &CreatePmdg777, "pmdg-777-200er", "77ER", RefuelBy::Client
    };

    [[maybe_unused]] const AircraftRegistration kPmdg777300ErRegistration{kPmdg777300ErDescriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg777FreighterRegistration{kPmdg777FreighterDescriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg777200LrRegistration{kPmdg777200LrDescriptor};
    [[maybe_unused]] const AircraftRegistration kPmdg777200ErRegistration{kPmdg777200ErDescriptor};
}
