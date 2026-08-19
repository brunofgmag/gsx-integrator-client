#include "Pmdg777.h"

#include "../simvars/SimVars.h"

#include <memory>
#include <utility>
#include "AircraftRegistry.h"
#include "../gsx/GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../pmdg/Pmdg777DataClient.h"
#include "../pmdg/PmdgTabletClient.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kSmartSwitchCaptLVar = "switch_554_a";
    constexpr auto kSmartSwitchFoLVar = "switch_773_a";
    constexpr double kSmartSwitchPressed = 100.0;

    constexpr int kDoorSlots = 16;
    constexpr int kMainDeckCargoDoor = 12;
    constexpr int kDoorStateOpen = 0;
    constexpr int kDoorStateClosing = 3;
    constexpr int kDoorStateOpening = 4;

    constexpr auto kTitle300Er = "777-300ER";
    constexpr auto kTitleFreighter = "777F";
    constexpr auto kTitle200Lr = "777-200LR";
    constexpr auto kTitle200Er = "777-200ER";

    bool IsCargo(const Pmdg777Variant variant)
    {
        return variant == Pmdg777Variant::Freighter;
    }

    PmdgAircraftSpec SpecFor(const Pmdg777Variant variant)
    {
        return {
            kDoorSlots,
            kMainDeckCargoDoor,
            IsCargo(variant),
            DoorBaseline::Unknown,
            {kSmartSwitchCaptLVar, kSmartSwitchFoLVar},
            [](double, const double max) { return max >= kSmartSwitchPressed; }
        };
    }
}

Pmdg777::Pmdg777(VariableGateway* variableGateway,
                 const AutomationStatus* status,
                 const Pmdg777Variant variant,
                 std::unique_ptr<Pmdg777DataGateway> data,
                 std::unique_ptr<PmdgTabletGateway> tablet)
    : PmdgAircraft(variableGateway, status, data.get(), std::move(tablet), SpecFor(variant)),
      variant_(variant),
      ownedData_(std::move(data))
{
    LOG_INFO("Profile loaded: %s", GetName());
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

int Pmdg777::DoorSlotFor(const GsxDoor door) const
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

DoorObservation Pmdg777::ObserveDoor(const int slot) const
{
    const int state = ownedData_->DoorState(slot);
    if (state < 0)
    {
        return DoorObservation::Unavailable;
    }

    if (state == kDoorStateClosing || state == kDoorStateOpening)
    {
        return DoorObservation::Moving;
    }

    return state == kDoorStateOpen ? DoorObservation::Open : DoorObservation::Closed;
}

void Pmdg777::ToggleDoor(const int slot)
{
    ownedData_->ToggleDoor(slot);
}

void Pmdg777::RefreshDoors()
{
}

bool Pmdg777::HasAircraftPower() const
{
    return ownedData_->ApuRunning() || ownedData_->ExtPowerConnected();
}

bool Pmdg777::GroundPowerConnected() const
{
    return ownedData_->ExtPowerConnected();
}

bool Pmdg777::GroundPowerPresent() const
{
    return ownedData_->ExtPowerAvailable() || ownedData_->ExtPowerConnected();
}

bool Pmdg777::ChocksSet() const
{
    return ownedData_->WheelChocksSet();
}

bool Pmdg777::HasVendorFlightPlan() const
{
    return ownedData_->HasFmcFlightPlan();
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

    std::unique_ptr<Aircraft> CreatePmdg777(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return std::make_unique<Pmdg777>(
            context.variableGateway, context.status, VariantFor(identity),
            std::make_unique<Pmdg777DataClient>(),
            std::make_unique<PmdgTabletClient>(context.commBusBridge));
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
