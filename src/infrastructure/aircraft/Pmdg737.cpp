#include "Pmdg737.h"

#include "../simvars/SimVars.h"

#include <memory>
#include <utility>
#include "AircraftRegistry.h"
#include "../logging/LogMacros.h"
#include "../pmdg/Pmdg737DataClient.h"
#include "../pmdg/PmdgTabletClient.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace simvars;

namespace
{
    constexpr auto kChocksLVar = "NGXWheelChocks";
    constexpr auto kSmartSwitchLVar = "switch_752_73X";
    constexpr double kSmartSwitchNeutral = 50.0;

    constexpr int kStateQueryTicks = 3;

    constexpr auto kTitlePax800 = "737-800 PAX";
    constexpr auto kTitleBcf800 = "737-800BCF";
    constexpr auto kTitleBdsf800 = "737-800BDSF";
    constexpr auto kTitleBbj2Short = "737-800 BB2";
    constexpr auto kTitleBbj2 = "737-800 BBJ2";

    bool IsCargo(const Pmdg737Variant variant)
    {
        return variant == Pmdg737Variant::Bcf800 || variant == Pmdg737Variant::Bdsf800;
    }

    const char* EfbDoorKey(const Pmdg737Door door)
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

    PmdgAircraftSpec SpecFor(const Pmdg737Variant variant)
    {
        return {
            static_cast<int>(Pmdg737Door::Count),
            static_cast<int>(Pmdg737Door::MainCargo),
            IsCargo(variant),
            DoorBaseline::Closed,
            {kSmartSwitchLVar},
            [](const double min, const double max)
            { return min < kSmartSwitchNeutral || max > kSmartSwitchNeutral; }
        };
    }
}

Pmdg737::Pmdg737(VariableGateway* variableGateway,
                 const AutomationStatus* status,
                 const Pmdg737Variant variant,
                 std::unique_ptr<Pmdg737DataGateway> data,
                 std::unique_ptr<PmdgTabletGateway> tablet)
    : PmdgAircraft(variableGateway, status, data.get(), std::move(tablet), SpecFor(variant)),
      variant_(variant),
      ownedData_(std::move(data))
{
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

int Pmdg737::DoorSlotFor(const GsxDoor door) const
{
    const std::optional<Pmdg737Door> target = DoorFor(door);

    return target.has_value() ? static_cast<int>(*target) : -1;
}

DoorObservation Pmdg737::ObserveDoor(const int slot) const
{
    const auto door = static_cast<Pmdg737Door>(slot);
    if (door == Pmdg737Door::Airstair)
    {
        return ObserveAirstair();
    }

    const char* key = EfbDoorKey(door);
    if (key == nullptr)
    {
        return DoorObservation::Unknown;
    }

    if (tablet_->DoorMoving(key))
    {
        return DoorObservation::Moving;
    }

    const std::optional<bool> reading = tablet_->DoorOpen(key);
    if (!reading.has_value())
    {
        return DoorObservation::Unknown;
    }

    return *reading ? DoorObservation::Open : DoorObservation::Closed;
}

DoorObservation Pmdg737::ObserveAirstair() const
{
    if (!ownedData_->AnyMainBusPowered())
    {
        return DoorObservation::Unknown;
    }

    return ownedData_->AirstairAnnunciator() ? DoorObservation::Open : DoorObservation::Closed;
}

void Pmdg737::ToggleDoor(const int slot)
{
    ownedData_->ToggleDoor(static_cast<Pmdg737Door>(slot));
}

void Pmdg737::RefreshDoors()
{
    if (++ticksSinceStateQuery_ >= kStateQueryTicks)
    {
        ticksSinceStateQuery_ = 0;
        tablet_->RequestState();
    }
}

bool Pmdg737::HasAircraftPower() const
{
    return ownedData_->AnyMainBusPowered();
}

bool Pmdg737::GroundPowerConnected() const
{
    return ownedData_->GroundPowerAvailable();
}

bool Pmdg737::ChocksSet() const
{
    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
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
